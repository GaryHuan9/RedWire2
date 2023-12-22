#include "Interface/Components.hpp"
#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"
#include "Utility/Functions.hpp"

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui.h"

namespace rw
{

LayerView::LayerView(Application& application) : Component(application)
{
	Float2 window_size(window.getSize());
	set_aspect_ratio(window_size.x / window_size.y);
}

LayerView::~LayerView() = default;

void LayerView::update(const Timer& timer)
{
	//TODO: do some kind of a fetching logic from whoever that owns the Layer
	if (current_layer == nullptr) return;

	draw_grid(window);
	draw_layer(window, *current_layer);
}

void LayerView::input_event(const sf::Event& event)
{
	Component::input_event(event);
	if (event.type != sf::Event::Resized) return;

	Float2 window_size(window.getSize());
	set_aspect_ratio(window_size.x / window_size.y);
}

Float2 LayerView::get_point(Float2 percent) const
{
	percent = percent * 2.0f - Float2(1.0f);
	return center + extend * percent;
}

sf::RenderStates LayerView::get_render_states() const
{
	float scale;
	Float2 origin;
	get_scale_origin(scale, origin);

	sf::RenderStates states;
	states.transform.translate(origin.x, origin.y);
	states.transform.scale(scale, scale);
	return states;
}

void LayerView::update_zoom()
{
	float shifted_zoom = zoom - ZoomLevelShift;

	zoom_gap = 1;
	zoom_level = static_cast<int32_t>(std::floor(shifted_zoom));
	for (int32_t i = 0; i < zoom_level; ++i) zoom_gap *= ZoomIncrement;

	zoom_scale = std::pow(static_cast<float>(ZoomIncrement), zoom);
	zoom_percent = shifted_zoom - static_cast<float>(zoom_level);
	extend = Float2(zoom_scale * aspect_ratio, zoom_scale);
}

void LayerView::get_scale_origin(float& scale, Float2& origin) const
{
	auto window_size = Float2(window.getSize());
	scale = window_size.x / 2.0f / extend.x; //This factor should be the same on both axes, using X here
	origin = window_size / 2.0f - center * scale;
}

void LayerView::draw_grid(sf::RenderWindow& window) const
{
	float scale;
	Float2 origin;
	get_scale_origin(scale, origin);

	auto window_size = Float2(window.getSize());
	Float2 min = get_min();
	Float2 max = get_max();

	auto drawer = [&](int32_t gap, float percent)
	{
		Int2 int_min = Float2::ceil(min / static_cast<float>(gap)) * gap;
		Int2 int_max = Float2::floor(max / static_cast<float>(gap)) * gap;

		sf::Color color(255, 255, 255, static_cast<int8_t>(GridLineAlpha * percent));
		if (color.a == 0) return;

		for (int32_t int_x = int_min.x; int_x <= int_max.x; int_x += gap)
		{
			float x = std::fma(static_cast<float>(int_x), scale, origin.x);

			vertices.emplace_back(sf::Vector2f(x, 0.0f), color);
			vertices.emplace_back(sf::Vector2f(x, window_size.y), color);
		}

		for (int32_t int_y = int_min.y; int_y <= int_max.y; int_y += gap)
		{
			float y = std::fma(static_cast<float>(int_y), scale, origin.y);

			vertices.emplace_back(sf::Vector2f(0.0f, y), color);
			vertices.emplace_back(sf::Vector2f(window_size.x, y), color);
		}
	};

	if (zoom_level >= 0)
	{
		drawer(zoom_gap * ZoomIncrement, zoom_percent);
		drawer(zoom_gap, 1.0f - zoom_percent);
	}
	else drawer(zoom_gap, 1.0f);

	window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
	vertices.clear();
}

void LayerView::draw_layer(sf::RenderWindow& window, const Layer& layer) const
{
	sf::RenderStates states_wire = get_render_states();
	sf::RenderStates states_static = states_wire;

	sf::Shader shader;
	shader.loadFromFile("rsc/Tiles/WireShader.frag", sf::Shader::Fragment);

	DrawContext context(window, states_wire, states_static);
	layer.draw(context, get_min(), get_max());
}

Controller::Controller(Application& application) : Component(application) {}

void Controller::initialize()
{
	layer = std::make_unique<Layer>();
	layer_view = application.find_component<LayerView>();
	layer_view->set_current_layer(layer.get());
}

void Controller::update(const Timer& timer)
{
	static constexpr std::array Tools = { "Move", "Remove", "Wire", "Bridge", "Transistor", "Inverter" };

	if (ImGui::Begin("Controller"))
	{
		int* pointer = reinterpret_cast<int*>(&selected_tool);
		ImGui::SliderInt("Tool", pointer, 0, Tools.size() - 1, Tools[selected_tool]);
	}

	ImGui::End();

	{
		Float2 mouse(sf::Mouse::getPosition(window));
		Float2 old_mouse_percent = mouse_percent;
		mouse_percent = Float2(mouse) / Float2(window.getSize());
		mouse_delta = mouse_percent - old_mouse_percent;
	}

	Int2 position;
	if (not try_get_mouse_position(position)) return;

	if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) selected_tool = 0;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) selected_tool = 1;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) selected_tool = 2;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) selected_tool = 3;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) selected_tool = 4;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) selected_tool = 5;

	if (selected_tool != 0)
	{
		sf::RenderStates states = layer_view->get_render_states();
		sf::RectangleShape cursor(sf::Vector2f(1.0f, 1.0f));

		cursor.setFillColor(sf::Color::Transparent);
		cursor.setOutlineColor(sf::Color(200, 200, 210));
		cursor.setOutlineThickness(0.05f);

		cursor.setPosition(static_cast<float>(position.x), static_cast<float>(position.y));
		window.draw(cursor, states);
	}

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
	{
		TileTag tile = layer->get(position);

		switch (selected_tool)
		{
			case 0:
			{
				Float2 old_mouse_percent = mouse_percent - mouse_delta;
				Float2 old_point = layer_view->get_point(old_mouse_percent);
				layer_view->set_point(mouse_percent, old_point);
				break;
			}
			case 1:
			{
				if (tile.type == TileType::Wire) Wire::erase(*layer, position);
				else if (tile.type == TileType::Bridge) Bridge::erase(*layer, position);
				else if (tile.type == TileType::Gate) Gate::erase(*layer, position);
				break;
			}
			case 2:
			{
				if (tile.type == TileType::None) Wire::insert(*layer, position);
				break;
			}
			case 3:
			{
				if (tile.type == TileType::None) Bridge::insert(*layer, position);
				break;
			}
			case 4:
			case 5:
			{
				Gate::Type type = selected_tool == 4 ? Gate::Type::Transistor : Gate::Type::Inverter;
				if (tile.type == TileType::None) Gate::insert(*layer, position, type, rotation);
				break;
			}
		}
	}
}

void Controller::input_event(const sf::Event& event)
{
	Component::input_event(event);

	if (event.type == sf::Event::MouseWheelScrolled)
	{
		float delta = event.mouseWheelScroll.delta / -32.0f;
		layer_view->change_zoom(delta, mouse_percent);
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
	{
		rotation = rotation.get_next();
	}
}

bool Controller::try_get_mouse_position(Int2& position) const
{
	if (not application.handle_mouse()) return false;
	position = Float2::floor(layer_view->get_point(mouse_percent));
	return true;
}

Debugger::Debugger(Application& application) : Component(application) {}

void Debugger::initialize()
{
	layer_view = application.find_component<LayerView>();
	controller = application.find_component<Controller>();
}

void Debugger::update(const Timer& timer)
{
	if (not ImGui::Begin("Debugger"))
	{
		ImGui::End();
		return;
	}

	if (controller == nullptr || layer_view == nullptr) return;
	if (controller->get_layer() == nullptr) return;
	Layer& layer = *controller->get_layer();

	if (Int2 position; controller->try_get_mouse_position(position))
	{
		TileTag tile = layer.get(position);
		ImGui::LabelText("Mouse Position", to_string(position).c_str());
		ImGui::LabelText("Tile Type", to_string(tile.type).c_str());

		if (tile.type != TileType::None)
		{
			ImGui::LabelText("Tile Index", to_string(tile.index).c_str());
		}
	}

	static int debug_wire = -1;
	ImGui::InputInt("Debug Wire", &debug_wire);

	const auto& wires = layer.get_list<Wire>();
	if (debug_wire >= 0 && wires.contains(Index(debug_wire)))
	{
		const Wire& wire = wires[Index(debug_wire)];
		sf::RenderStates states = layer_view->get_render_states();
		sf::RectangleShape shape(sf::Vector2f(0.5f, 0.5f));

		shape.setFillColor(sf::Color::Green);

		for (Int2 position : wire.positions)
		{
			Float2 offset = Float2(position) + Float2(0.5f);
			shape.setPosition(offset.x, offset.y);
			window.draw(shape, states);
		}

		shape.setFillColor(sf::Color::Cyan);

		for (Int2 position : wire.bridges)
		{
			Float2 offset(position);
			shape.setPosition(offset.x, offset.y);
			window.draw(shape, states);
		}
	}

	static int debug_gate = -1;
	ImGui::InputInt("Debug Gate", &debug_gate);

	const auto& gates = layer.get_list<Gate>();
	if (debug_gate >= 0 && gates.contains(Index(debug_gate)))
	{
		const Gate& gate = gates[Index(debug_gate)];
		ImGui::LabelText("Gate Type", gate.type == Gate::Type::Transistor ? "Transistor" : "Inverter");
		ImGui::LabelText("Gate Rotation", gate.rotation.to_string());
		ImGui::LabelText("Output Index", to_string(gate.output_index()).c_str());
		ImGui::LabelText("Input Index [0]", to_string(gate.input_indices()[0]).c_str());
		ImGui::LabelText("Input Index [1]", to_string(gate.input_indices()[1]).c_str());
		ImGui::LabelText("Input Index [2]", to_string(gate.input_indices()[2]).c_str());
	}

	ImGui::End();
}

}
