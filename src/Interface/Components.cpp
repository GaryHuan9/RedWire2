#include "Interface/Components.hpp"
#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"
#include "Functional/Engine.hpp"
#include "Functional/Drawing.hpp"
#include "Utility/Functions.hpp"

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui.h"

namespace rw
{

Controller::Controller(Application& application) : Component(application) {}

void Controller::initialize()
{
	layer = std::make_unique<Layer>();
}

void Controller::update(const Timer& timer) {}

LayerView::LayerView(Application& application) :
	Component(application),
	shader_quad(std::make_unique<sf::Shader>()),
	shader_wire(std::make_unique<sf::Shader>()),
	draw_context(std::make_unique<DrawContext>(shader_quad.get(), shader_wire.get()))
{
	Float2 window_size(window.getSize());
	set_aspect_ratio(window_size.x / window_size.y);

	shader_quad->loadFromFile("rsc/Tiles/Quad.vert", "rsc/Tiles/Tile.frag");
	shader_wire->loadFromFile("rsc/Tiles/Wire.vert", "rsc/Tiles/Tile.frag");
}

LayerView::~LayerView() = default;

void LayerView::initialize()
{
	controller = application.find_component<Controller>();
}

void LayerView::update(const Timer& timer)
{
	Layer* layer = controller->get_layer();
	if (layer == nullptr) return;

	if (dirty) update_grid();
	draw_grid();
	draw_layer(*layer);
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

void LayerView::get_scale_origin(float& scale, Float2& origin) const
{
	auto window_size = Float2(window.getSize());
	scale = window_size.x / 2.0f / extend.x; //This factor should be the same on both axes, using X here
	origin = window_size / 2.0f - center * scale;
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

void LayerView::update_grid()
{
	float scale;
	Float2 origin;
	get_scale_origin(scale, origin);

	auto window_size = Float2(window.getSize());
	Float2 min = get_min();
	Float2 max = get_max();

	vertices.clear();

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

	dirty = false;
}

void LayerView::draw_grid() const
{
	window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
}

void LayerView::draw_layer(const Layer& layer) const
{
	Float2 scale = Float2(1.0f, -1.0f) / extend;
	Float2 origin = -center * scale;

	shader_quad->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_quad->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));

	shader_wire->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_wire->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));

	const void* states_data;
	size_t states_size;
	layer.get_engine().get_states(states_data, states_size);
	draw_context->update_wire_states(states_data, states_size);

	layer.draw(*draw_context, get_min(), get_max());
	draw_context->clear();
}

Cursor::Cursor(Application& application) :
	Component(application),
	shape_outline(std::make_unique<sf::RectangleShape>()),
	shape_fill(std::make_unique<sf::RectangleShape>())
{
	sf::Color color(230, 225, 240);

	shape_outline->setFillColor(sf::Color::Transparent);
	shape_outline->setOutlineThickness(0.03f);
	shape_outline->setOutlineColor(color);
	shape_fill->setFillColor(color);
}

void Cursor::initialize()
{
	controller = application.find_component<Controller>();
	layer_view = application.find_component<LayerView>();
}

void Cursor::update(const Timer& timer)
{
	Float2 mouse(sf::Mouse::getPosition(window));
	Float2 old_mouse_percent = mouse_percent;
	mouse_percent = Float2(mouse) / Float2(window.getSize());
	mouse_delta = mouse_percent - old_mouse_percent;

	Layer* layer = controller->get_layer();

	if (ImGui::Begin("Cursor") && layer != nullptr) update_interface();
	ImGui::End();

	if (Int2 position; layer_view != nullptr && try_get_mouse_position(position)) execute(position);
}

void Cursor::input_event(const sf::Event& event)
{
	Component::input_event(event);

	if (event.type == sf::Event::MouseWheelScrolled)
	{
		float delta = event.mouseWheelScroll.delta / -32.0f;
		layer_view->change_zoom(delta, mouse_percent);
	}

	if (event.type == sf::Event::KeyPressed || event.type == sf::Event::MouseButtonPressed) update_input_event(event);
}

bool Cursor::try_get_mouse_position(Int2& position) const
{
	if (not application.handle_mouse()) return false;
	position = Float2::floor(layer_view->get_point(mouse_percent));
	return true;
}

void Cursor::update_interface()
{
	static constexpr std::array ToolNames = { "Mouse", "Wire Placement", "Port Placement", "Tile Removal" };
	static constexpr std::array PortNames = { "Transistor", "Inverter", "Bridge" };

	{
		Int2 position;
		bool no_position = not try_get_mouse_position(position);
		if (no_position) ImGui::LabelText("Mouse Position", "Not Available");
		else ImGui::LabelText("Mouse Position", to_string(position).c_str());
	}

	int tool = static_cast<int>(selected_tool);
	ImGui::Combo("Tools", &tool, ToolNames.data(), ToolNames.size());
	selected_tool = static_cast<ToolType>(tool);

	switch (selected_tool)
	{
		case ToolType::WirePlacement:
		{
			break;
		}
		case ToolType::PortPlacement:
		{
			int port = static_cast<int>(selected_port);
			ImGui::SliderInt("Port", &port, 0, PortNames.size() - 1, PortNames[port]);
			selected_port = static_cast<PortType>(port);

			int rotation = selected_rotation.get_value();
			ImGui::SliderInt("Rotation", &rotation, 0, 3, selected_rotation.to_string());
			selected_rotation = TileRotation(static_cast<TileRotation::Value>(rotation));
			break;
		}
		case ToolType::TileRemoval:
		{
			break;
		}
		default: break;
	}
}

void Cursor::update_input_event(const sf::Event& event)
{
	if (event.type == sf::Event::KeyPressed)
	{
		const auto& key = event.key;

		if (key.code == sf::Keyboard::E) selected_tool = ToolType::WirePlacement;

		if (key.code == sf::Keyboard::R && selected_tool == ToolType::PortPlacement && selected_port != PortType::Bridge)
		{
			selected_rotation = selected_rotation.get_next();
		}

		if (sf::Keyboard::Num1 <= key.code && key.code <= sf::Keyboard::Num3)
		{
			selected_tool = ToolType::PortPlacement;

			if (key.code == sf::Keyboard::Num1) selected_port = PortType::Transistor;
			else if (key.code == sf::Keyboard::Num2) selected_port = PortType::Inverter;
			else if (key.code == sf::Keyboard::Num3) selected_port = PortType::Bridge;
		}
	}
	else
	{
		assert(event.type == sf::Event::MouseButtonPressed);
		const auto& mouse = event.mouseButton;

		if (mouse.button == sf::Mouse::Right) selected_tool = ToolType::Mouse;
	}
}

void Cursor::execute(Int2 position)
{
	bool button = sf::Mouse::isButtonPressed(sf::Mouse::Left);
	if (not button) drag_type = DragType::None;

	if (selected_tool == ToolType::Mouse)
	{
		if (button)
		{
			Float2 old_mouse_percent = mouse_percent - mouse_delta;
			Float2 old_point = layer_view->get_point(old_mouse_percent);
			layer_view->set_point(mouse_percent, old_point);
		}

		return;
	}

	if (button) execute_drag(position);
	Int2 draw_position = position;

	sf::RenderStates render_states = layer_view->get_render_states();

	if (selected_tool == ToolType::WirePlacement)
	{
		if (button) draw_position = place_wire(position);
	}
	else
	{
		if (button) draw_position = place_port(position);

		if (selected_port != PortType::Bridge)
		{
			constexpr float Extend = 0.15f;
			constexpr float Offset = 0.5f - Extend;

			Float2 origin = Float2(draw_position) + Float2(0.5f);
			Int2 direction = selected_rotation.get_direction();
			Float2 center = origin + Float2(direction) * Offset;
			center -= Float2(Extend, Extend);

			shape_fill->setPosition(center.x, center.y);
			shape_fill->setSize({ Extend * 2.0f, Extend * 2.0f });
			window.draw(*shape_fill, render_states);
		}
	}

	{
		Float2 center(draw_position);
		shape_outline->setPosition(center.x, center.y);
		shape_outline->setSize({ 1.0f, 1.0f });
		window.draw(*shape_outline, render_states);
	}
}

void Cursor::execute_drag(Int2 position)
{
	if (drag_type == DragType::None)
	{
		drag_type = DragType::Origin;
		drag_origin = position;
	}
	else if (drag_type == DragType::Origin && position != drag_origin)
	{
		Int2 delta = position - drag_origin;
		delta = delta.max(-delta); //Absolute value
		drag_type = delta.x > delta.y ? DragType::Horizontal : DragType::Vertical;
	}
}

Int2 Cursor::place_wire(Int2 position)
{
	assert(drag_type != DragType::None);
	Layer& layer = *controller->get_layer();

	if (drag_type == DragType::Origin)
	{
		if (layer.has(drag_origin, TileType::None)) Wire::insert(layer, drag_origin);
		return drag_origin;
	}

	assert(drag_type == DragType::Horizontal || drag_type == DragType::Vertical);

	Float2 old_mouse_percent = mouse_percent - mouse_delta;
	Float2 old_point = layer_view->get_point(old_mouse_percent);
	Int2 old_position = Float2::floor(old_point);

	auto drag = drag_type == DragType::Horizontal ?
	            std::make_pair(old_position.x, position.x) :
	            std::make_pair(old_position.y, position.y);

	if (drag.first > drag.second) std::swap(drag.first, drag.second);

	for (int32_t i = drag.first; i <= drag.second; ++i)
	{
		Int2 current(drag_origin.x, i);
		if (drag_type == DragType::Horizontal) current = Int2(i, drag_origin.y);
		if (layer.has(current, TileType::None)) Wire::insert(layer, current);
	}

	return drag_type == DragType::Horizontal ? Int2(position.x, drag_origin.y) : Int2(drag_origin.x, position.y);
}

Int2 Cursor::place_port(Int2 position)
{
	assert(drag_type != DragType::None);
	if (position != drag_origin) return drag_origin;
	Layer& layer = *controller->get_layer();

	{
		TileType type = layer.get(position).type;
		if (type == TileType::Wire) Wire::erase(layer, position);
		else if (type != TileType::None) return position;
	}

	if (selected_port == PortType::Bridge)
	{
		Bridge::insert(layer, position);
	}
	else
	{
		auto type = Gate::Type::Transistor;
		if (selected_port == PortType::Inverter) type = Gate::Type::Inverter;
		Gate::insert(layer, position, type, selected_rotation);
	}

	return position;
}

Debugger::Debugger(Application& application) : Component(application) {}

void Debugger::update(const Timer& timer)
{
	if (not ImGui::Begin("Debugger"))
	{
		ImGui::End();
		return;
	}

	auto* controller = application.find_component<Controller>();
	auto* layer_view = application.find_component<LayerView>();
	auto* cursor = application.find_component<Cursor>();

	if (controller == nullptr) return;
	if (layer_view == nullptr) return;
	if (cursor == nullptr) return;

	if (controller->get_layer() == nullptr) return;
	Layer& layer = *controller->get_layer();

	if (Int2 position; cursor->try_get_mouse_position(position))
	{
		TileTag tile = layer.get(position);
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

Updater::Updater(Application& application) : Component(application) {}

void Updater::initialize()
{
	controller = application.find_component<Controller>();
}

void Updater::update(const Timer& timer)
{
	Layer* layer = controller->get_layer();

	if (ImGui::Begin("Updater") && layer != nullptr) update_interface();
	ImGui::End();

	if (layer == nullptr) return;

	float delta_time = Timer::as_float(timer.frame_time());
	update(delta_time, layer->get_engine());

	last_display_time += delta_time;
	if (last_display_time >= 1.0f) update_display();
}

void Updater::update_interface()
{
	ImGui::SeparatorText("Control");

	{
		static constexpr std::array Names = { "Per Second", "Per Frame", "Manual", "Maximum" };
		int type = static_cast<int>(selected_type);
		ImGui::Combo("Type", &type, Names.data(), Names.size());

		Type old_type = selected_type;
		selected_type = static_cast<Type>(type);

		if (selected_type != old_type)
		{
			remain_count = 0;
			dropped_count = 0;
			per_second_error = 0.0f;
			executed = {};
			update_display();
		}
	}

	{
		static constexpr uint32_t TimeBudgetMin = 1;
		static constexpr uint32_t TimeBudgetMax = 100;
		auto budget = std::chrono::duration_cast<std::chrono::milliseconds>(time_budget).count();
		ImGui::DragScalar("Time Budget", ImGuiDataType_U32, &budget, 1.0f, &TimeBudgetMin, &TimeBudgetMax, "%u ms");
		time_budget = as_duration(budget);
	}

	if (selected_type != Type::Maximum)
	{
		ImGui::DragScalar("Target Count", ImGuiDataType_U32, &selected_count);
	}

	bool paused = selected_pause;
	ImGui::BeginDisabled(paused);
	if (ImGui::Button("Pause")) pause();
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::BeginDisabled(not paused);
	if (ImGui::Button("Resume")) resume();
	ImGui::EndDisabled();

	if (selected_type == Type::Manual)
	{
		ImGui::SameLine();
		ImGui::BeginDisabled(remain_count > 0);

		if (ImGui::Button("Update"))
		{
			remain_count = selected_count;
			executed = {};
			update_display();
		}

		ImGui::EndDisabled();
	}

	ImGui::SeparatorText("Statistics");

	{
		const char* display = display_updates_per_second.c_str();
		if (display_updates_per_second.empty()) display = "0";
		ImGui::LabelText("Achieved Updates Per Second", display);
	}

	if (not display_dropped_update_count.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
		ImGui::LabelText("Dropped Update Count", display_dropped_update_count.c_str());
		ImGui::PopStyleColor();
	}

	if (selected_type == Type::Manual && remain_count + executed.count > 0)
	{
		auto total = static_cast<float>(remain_count + executed.count);
		float progress = static_cast<float>(executed.count) / total;

		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::TextUnformatted("Manual Update Progress");
	}
}

void Updater::update_display()
{
	last_display_time = 0.0f;
	display_updates_per_second.clear();
	display_dropped_update_count.clear();

	auto to_string = [](uint64_t value) { return std::to_string(value); };

	if (executed.count > 0)
	{
		auto seconds = std::chrono::duration<float>(executed.duration);
		float ratio = static_cast<float>(executed.count) / seconds.count();
		display_updates_per_second = to_string(static_cast<uint64_t>(ratio));

		if (selected_type != Type::Manual) executed = {};
	}

	if (dropped_count > 0)
	{
		display_dropped_update_count = to_string(dropped_count);
		dropped_count = 0;
	}
}

void Updater::update(float delta_time, Engine& engine)
{
	if (selected_pause) return;

	switch (selected_type)
	{
		case Type::PerSecond:
		{
			float count = delta_time * static_cast<float>(selected_count);
			auto new_count = static_cast<uint64_t>(count);

			if (per_second_error >= 1.0f)
			{
				per_second_error -= 1.0f;
				++new_count;
			}

			remain_count = execute(engine, remain_count + new_count);
			per_second_error += count - static_cast<float>(new_count);

			if (remain_count > selected_count)
			{
				dropped_count += remain_count - selected_count;
				remain_count = selected_count;
			}

			break;
		}
		case Type::PerFrame:
		{
			dropped_count += execute(engine, selected_count);
			break;
		}
		case Type::Manual:
		{
			uint64_t old_count = remain_count;
			remain_count = execute(engine, remain_count);
			if (remain_count == 0 && old_count != 0) update_display();

			break;
		}
		case Type::Maximum:
		{
			execute(engine, std::numeric_limits<uint64_t>::max());
			break;
		}
	}
}

uint32_t Updater::execute(Engine& engine, uint64_t count)
{
	if (count == 0) return 0;

	Duration budget = time_budget;
	ExecutePair total;

	while (true)
	{
		auto attempt_budget = budget < as_duration(1) ? budget : budget / 2;
		uint64_t attempt = attempt_budget / last_execute_rate;
		attempt = std::clamp(attempt, uint64_t(1), count);

		auto start = Clock::now();
		for (uint64_t i = 0; i < attempt; ++i) engine.update();

		Duration elapsed = Clock::now() - start;
		total += ExecutePair(elapsed, attempt);
		count -= attempt;

		if (count == 0 || elapsed >= budget) break;
		last_execute_rate = total.duration / total.count;
		budget -= elapsed;
	}

	executed += total;
	return count;
}

}
