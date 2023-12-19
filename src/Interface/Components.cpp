#include "Interface/Components.hpp"
#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui.h"

#include <sstream>

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
	static constexpr std::array Tools = { "Move", "Remove", "Wire", "Bridge" };

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

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) selected_tool = 0;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) selected_tool = 1;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) selected_tool = 2;
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) selected_tool = 3;

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
				if (tile.type == TileType::Bridge) Bridge::erase(*layer, position);
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
		}
	}
}

void Controller::input_event(const sf::Event& event)
{
	Component::input_event(event);

	if (event.type != sf::Event::MouseWheelScrolled) return;
	float delta = event.mouseWheelScroll.delta / -32.0f;
	layer_view->change_zoom(delta, mouse_percent);
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
		auto to_string = []<class T>(const T& value)
		{
			std::stringstream stream;
			stream << value;
			return stream.str();
		};

		TileTag tile = layer.get(position);
		ImGui::LabelText("Mouse Position", to_string(position).c_str());
		ImGui::LabelText("Tile Type", tile.type.to_string());

		if (tile.type != TileType::None)
		{
			ImGui::LabelText("Tile Index", std::to_string(tile.index).c_str());
		}
	}

	ImGui::End();
}

}
