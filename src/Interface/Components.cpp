#include "Interface/Components.hpp"
#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"
#include "Functional/Engine.hpp"
#include "Functional/Drawing.hpp"
#include "Utility/Functions.hpp"

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace rw
{

Controller::Controller(Application& application) : Component(application) {}

void Controller::initialize()
{
	layer = std::make_unique<Layer>();
	std::string("test.rw2").copy(path_buffer.data(), path_buffer.size());
}

void Controller::update()
{
	if (layer == nullptr)
	{
		if (imgui_begin("Controller")) ImGui::End();
		return;
	}

	if (imgui_begin("Controller"))
	{
		update_interface();
		ImGui::End();
	}
}

static void save_layer(const fs::path& path, const std::unique_ptr<Layer>& layer, const LayerView& layer_view)
{
	fs::create_directories(path.parent_path());

	auto stream = std::make_shared<std::ofstream>(path, std::ios::binary | std::ios::trunc);
	if (not stream->good()) throw std::runtime_error("Unable to open stream.");

	BinaryWriter writer(stream);
	writer << static_cast<uint32_t>(3);
	writer << *layer << layer_view;
}

static void load_layer(const fs::path& path, std::unique_ptr<Layer>& layer, LayerView& layer_view)
{
	auto stream = std::make_shared<std::ifstream>(path, std::ios::binary);
	if (not stream->good()) throw std::runtime_error("Unable to open stream.");

	BinaryReader reader(stream);
	uint32_t version;
	reader >> version;
	if (version != 3) throw std::runtime_error("Unrecognized version.");

	layer = std::make_unique<Layer>();
	reader >> *layer >> layer_view;
}

static void new_layer(std::unique_ptr<Layer>& layer, LayerView& layer_view)
{
	layer = std::make_unique<Layer>();
	layer_view.reset();
}

void Controller::update_interface()
{
	assert(layer != nullptr);

	ImGui::SeparatorText("Serialization");
	ImGui::InputText("File Name", path_buffer.data(), path_buffer.size());
	fs::path path = fs::path("saves") / std::string(path_buffer.data());
	imgui_tooltip("Name of the file to save as or load from");

	if (selected_action == ActionType::None)
	{
		bool has_file = fs::is_regular_file(path);

		ImGui::BeginDisabled(fs::is_directory(path));
		if (ImGui::Button(has_file ? "Overwrite" : "Save")) selected_action = ActionType::Save;
		imgui_tooltip("Save current Board as a file on disk. May overwrite previous saves");

		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(not has_file);
		if (ImGui::Button("Load")) selected_action = ActionType::Load;
		imgui_tooltip("Load a file from disk and replace (i.e. erase) the current board");

		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("New")) selected_action = ActionType::New;
		imgui_tooltip("Erase everything on the current board and starts from scratch");
	}
	else
	{
		if (ImGui::Button("Confirm"))
		{
			auto& layer_view = *application.find_component<LayerView>();

			if (selected_action == ActionType::Save) save_layer(path, layer, layer_view);
			else if (selected_action == ActionType::Load) load_layer(path, layer, layer_view);
			else if (selected_action == ActionType::New) new_layer(layer, layer_view);

			selected_action = ActionType::None;
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel")) selected_action = ActionType::None;
	}
}

LayerView::LayerView(Application& application) :
	Component(application), render_states(std::make_unique<sf::RenderStates>()),
	draw_context(std::make_unique<DrawContext>(application.get_shaders()))
{
	Float2 window_size(window.getSize());
	set_aspect_ratio(window_size.x / window_size.y);
}

LayerView::~LayerView() = default;

void LayerView::initialize()
{
	controller = application.find_component<Controller>();
	reset();
}

void LayerView::update()
{
	Layer* layer = controller->get_layer();
	if (layer == nullptr) return;

	if (dirty)
	{
		update_render_states();
		update_grid();
		dirty = false;
	}

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

void LayerView::get_scale_origin(Float2& scale, Float2& origin) const
{
	Float2 window_extend = Float2(window.getSize()) / 2.0f;
	float scale_x = window_extend.x / extend.x;

	scale = Float2(scale_x, -scale_x);
	origin = window_extend - center * scale;
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
	Float2 scale;
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

		auto alpha = static_cast<uint8_t>(GridLineAlpha * percent);
		if (alpha == 0) return;

		sf::Color color(255, 255, 255, alpha);

		for (int32_t int_x = int_min.x; int_x <= int_max.x; int_x += gap)
		{
			float x = std::fma(static_cast<float>(int_x), scale.x, origin.x);

			vertices.emplace_back(sf::Vector2f(x, 0.0f), color);
			vertices.emplace_back(sf::Vector2f(x, window_size.y), color);
		}

		for (int32_t int_y = int_min.y; int_y <= int_max.y; int_y += gap)
		{
			float y = std::fma(static_cast<float>(int_y), scale.y, origin.y);

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
}

void LayerView::update_render_states()
{
	Float2 scale;
	Float2 origin;
	get_scale_origin(scale, origin);

	sf::Transform transform;
	transform.translate(origin.x, origin.y);
	transform.scale(scale.x, scale.y);
	render_states->transform = transform;
}

void LayerView::draw_grid() const
{
	window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
}

void LayerView::draw_layer(const Layer& layer) const
{
	draw_context->set_view(center, extend);

	const void* states_data;
	size_t states_size;
	layer.get_engine().get_states(states_data, states_size);
	draw_context->set_wire_states(states_data, states_size);

	layer.draw(*draw_context, get_min(), get_max());
	draw_context->clear();
}

Cursor::Cursor(Application& application) : Component(application), tools() {}

void Cursor::initialize()
{
	controller = application.find_component<Controller>();
	layer_view = application.find_component<LayerView>();

	//Create tools at the end of initialization since some of them might look for components
	tools.emplace_back(std::make_unique<Cursor::MouseTool>(*this));
	tools.emplace_back(std::make_unique<Cursor::WireTool>(*this));
	tools.emplace_back(std::make_unique<Cursor::DeviceTool>(*this));
	tools.emplace_back(std::make_unique<Cursor::RemovalTool>(*this));
	tools.emplace_back(std::make_unique<Cursor::ClipboardTool>(*this));
}

void Cursor::update()
{
	Float2 mouse(sf::Mouse::getPosition(window));
	mouse_percent = mouse / Float2(window.getSize());
	mouse_percent.y = 1.0f - mouse_percent.y;

	last_mouse_point = mouse_point;
	mouse_point = layer_view->get_point(mouse_percent);

	if (controller->get_layer() == nullptr)
	{
		if (imgui_begin("Cursor")) ImGui::End();
		return;
	}

	Tool& tool = *tools[selected_tool];
	uint32_t old_tool = selected_tool;

	if (imgui_begin("Cursor"))
	{
		update_interface();
		tool.update_interface();
		ImGui::End();
	}

	if (application.handle_keyboard()) update_panning();
	if (application.handle_mouse()) tool.update_mouse();
	if (old_tool != selected_tool) tool.deactivate();
}

void Cursor::input_event(const sf::Event& event)
{
	Component::input_event(event);
	if (controller->get_layer() == nullptr) return;

	if (event.type == sf::Event::MouseWheelScrolled)
	{
		float delta = event.mouseWheelScroll.delta / -32.0f;
		layer_view->change_zoom(delta, mouse_percent);
	}

	Tool& tool = *tools[selected_tool];
	tool.input_event(event);

	for (uint32_t i = 0; i < tools.size(); ++i)
	{
		if (not tools[i]->request_activation(event)) continue;

		if (i != selected_tool)
		{
			tool.deactivate();
			selected_tool = i;
		}

		break;
	}
}

void Cursor::update_interface()
{
	{
		Int2 position;
		bool no_position = not try_get_mouse_position(position);
		if (no_position) ImGui::LabelText("Mouse Position", "Not Available");
		else ImGui::LabelText("Mouse Position", to_string(position).c_str());

		ImGui::DragFloat("Pan Sensitivity", &selected_pan_sensitivity, 0.1f, 0.0f, 1.0f);
		imgui_tooltip("How fast (in horizontal screen percentage) the viewport moves when [WASD] keys are pressed");
	}

	{
		static constexpr std::array ToolNames = { "Mouse", "Wire Placement", "Port Placement", "Tile Removal", "Clipboard" };

		int tool = static_cast<int>(selected_tool);
		ImGui::Combo("Tool", &tool, ToolNames.data(), ToolNames.size());
		selected_tool = static_cast<uint32_t>(tool);

		imgui_tooltip(
			"Currently selected cursor tool. Can also be switched with button shortcuts: Mouse = [RMB], Wire = [E], "
			"Transistor = [Num1], Inverter = [Num2], Bridge = [Num3], Removal = [Q], Cut = [X], Copy = [C], Paste = [V]"
		);
	}
}

void Cursor::update_panning()
{
	Int2 view_input;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) view_input += Int2(0, 1);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) view_input += Int2(0, -1);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) view_input += Int2(-1, 0);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) view_input += Int2(1, 0);

	if (view_input == Int2(0)) return;
	float delta_time = Timer::as_float(application.get_timer().frame_time());
	float speed = delta_time * selected_pan_sensitivity;
	Float2 delta = view_input.normalized() * speed;

	//Move based on relative screen percentage
	Float2 reference = layer_view->get_point(Float2(0.0f));
	delta.y *= layer_view->get_aspect_ratio();
	layer_view->set_point(-delta, reference);
	mouse_point = layer_view->get_point(mouse_percent);
}

void Cursor::Tool::update_mouse()
{
	Int2 size = get_placement_size();
	Float2 offset = Float2(size - Int2(1)) / 2.0f;
	Int2 position = Float2::floor(cursor.mouse_point - offset);

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (drag_type == DragType::None) drag_origin = position;

		if (restrict_drag_axis())
		{
			Int2 delta = (position - drag_origin) / size;
			position = delta * size + drag_origin;

			if (std::abs(delta.x) >= std::abs(delta.y))
			{
				drag_type = DragType::Horizontal;
				drag_position = Int2(position.x, drag_origin.y);
			}
			else
			{
				drag_type = DragType::Vertical;
				drag_position = Int2(drag_origin.x, position.y);
			}

			position = drag_position;
		}
		else
		{
			drag_type = DragType::Free;
			drag_position = position;
		}
	}
	else
	{
		if (mouse_pressed())
		{
			Layer* layer = cursor.controller->get_layer();
			assert(layer != nullptr);
			commit(*layer);
		}

		drag_type = DragType::None;
	}

	update(position);
}

void Cursor::Tool::draw_rectangle(Float2 center, Float2 extend, uint32_t color)
{
	Float2 min = center - extend;
	Float2 max = center + extend;
	std::array<sf::Vertex, 4> vertices = { sf::Vertex(sf::Vector2f(min.x, min.y), sf::Color(color)),
	                                       sf::Vertex(sf::Vector2f(max.x, min.y), sf::Color(color)),
	                                       sf::Vertex(sf::Vector2f(max.x, max.y), sf::Color(color)),
	                                       sf::Vertex(sf::Vector2f(min.x, max.y), sf::Color(color)) };

	auto render_states = cursor.layer_view->get_render_states();
	cursor.window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Quads, render_states);
}

static std::array<sf::Vertex, 5> get_vertices_border(Bounds bounds, bool highlight)
{
	sf::Color color(make_color(230, 225, 240, highlight ? 255 : 200));
	Float2 min(bounds.get_min());
	Float2 max(bounds.get_max());

	return { sf::Vertex(sf::Vector2f(min.x, min.y), color), sf::Vertex(sf::Vector2f(max.x, min.y), color),
	         sf::Vertex(sf::Vector2f(max.x, max.y), color), sf::Vertex(sf::Vector2f(min.x, max.y), color),
	         sf::Vertex(sf::Vector2f(min.x, min.y), color) };
}

void Cursor::Tool::draw_selection(Bounds bounds, uint32_t color)
{
	draw_rectangle(bounds.center(), bounds.extend(), color);

	auto vertices = get_vertices_border(bounds, mouse_pressed());
	auto render_states = cursor.layer_view->get_render_states();
	cursor.window.draw(vertices.data(), 5, sf::PrimitiveType::LineStrip, render_states);
}

void Cursor::Tool::draw_removal(Bounds bounds)
{
	draw_selection(bounds, make_color(220, 10, 30, 50));

	auto vertices = get_vertices_border(bounds, mouse_pressed());
	auto render_states = cursor.layer_view->get_render_states();
	std::swap(vertices[1], vertices[2]);
	cursor.window.draw(vertices.data(), 4, sf::PrimitiveType::Lines, render_states);
}

bool Cursor::MouseTool::request_activation(const sf::Event& event)
{
	return event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right;
}

void Cursor::MouseTool::update(Int2 position)
{
	if (mouse_pressed())
	{
		cursor.layer_view->set_point(cursor.mouse_percent, cursor.last_mouse_point);
		cursor.mouse_point = cursor.layer_view->get_point(cursor.mouse_percent);
	}

	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		Layer* layer = cursor.controller->get_layer();
		TileTag tile = layer->get(position);
		if (tile.type == TileType::Wire) layer->get_engine().toggle_wire_strong_powered(tile.index);
	}
}

bool Cursor::WireTool::request_activation(const sf::Event& event)
{
	return event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::E;
}

void Cursor::WireTool::update_interface()
{
	ImGui::Checkbox("Auto Bridge Placement", &selected_auto_bridge);
	imgui_tooltip("Whether to automatically place bridges at wire junctions (i.e. tiles with more than two wire neighbors)");
}

void Cursor::WireTool::update(Int2 position)
{
	Bounds bounds(position);
	if (mouse_pressed()) bounds = Bounds::encapsulate(drag_origin, position);
	draw_selection(bounds, Wire::ColorUnpowered);
}

void Cursor::WireTool::commit(Layer& layer)
{
	bool horizontal = drag_type == DragType::Horizontal;
	auto drag = horizontal ? std::make_pair(drag_origin.x, drag_position.x)
	                       : std::make_pair(drag_origin.y, drag_position.y);

	if (drag.first > drag.second) std::swap(drag.first, drag.second);
	Int2 other_axis = horizontal ? Int2(0, 1) : Int2(1, 0);

	for (int32_t i = drag.first; i <= drag.second; ++i)
	{
		Int2 current = horizontal ? Int2(i, drag_origin.y) : Int2(drag_origin.x, i);
		TileType type = layer.get(current).type;

		if (selected_auto_bridge)
		{
			if (type != TileType::None && type != TileType::Wire) continue;
			bool bridge = layer.has(current + other_axis, TileType::Wire) ||
			              layer.has(current - other_axis, TileType::Wire);

			if (bridge)
			{
				if (type == TileType::Wire)
				{
					Wire::erase(layer, current);
					type = TileType::None;
				}

				assert(type == TileType::None);
				Bridge::insert(layer, current);
				continue;
			}
		}

		if (type == TileType::None) Wire::insert(layer, current);
	}
}

void Cursor::DeviceTool::update_interface()
{
	int type = static_cast<int>(selected_type);
	static constexpr std::array TypeNames = { "Transistor", "Inverter", "Bridge" };
	ImGui::SliderInt("Device", &type, 0, TypeNames.size() - 1, TypeNames[type]);
	selected_type = static_cast<Type>(type);

	imgui_tooltip(
		"Currently selected device to place. Can also be switched with button shortcuts: "
		"Transistor = [Num1], Inverter = [Num2], Bridge = [Num3]"
	);

	if (selected_type != Type::Bridge)
	{
		int rotation = selected_rotation.get_value();
		ImGui::SliderInt("Rotation", &rotation, 0, TileRotation::Count - 1, selected_rotation.to_string());
		selected_rotation = TileRotation(static_cast<TileRotation::Value>(rotation));
		imgui_tooltip("Rotation of the new device placed. Use [R] to quickly switch to the next rotation.");
	}
}

bool Cursor::DeviceTool::request_activation(const sf::Event& event)
{
	if (event.type != sf::Event::KeyPressed) return false;

	auto code = event.key.code;
	switch (code)
	{
		case sf::Keyboard::Num1:
		case sf::Keyboard::Num2:
		case sf::Keyboard::Num3:
		{
			if (code == sf::Keyboard::Num1) selected_type = Type::Transistor;
			if (code == sf::Keyboard::Num2) selected_type = Type::Inverter;
			if (code == sf::Keyboard::Num3) selected_type = Type::Bridge;
			return true;
		}
		default: return false;
	}
}

void Cursor::DeviceTool::input_event(const sf::Event& event)
{
	Tool::input_event(event);
	if (mouse_pressed()) return;

	if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::R) return;
	selected_rotation = selected_rotation.get_next();
}

void Cursor::DeviceTool::update(Int2 position)
{
	uint32_t color;

	if (selected_type == Type::Transistor) color = Gate::ColorTransistor;
	else if (selected_type == Type::Inverter) color = Gate::ColorInverter;
	else if (selected_type == Type::Bridge) color = Bridge::Color;
	else throw std::domain_error("Bad PortType.");

	if (mouse_pressed()) position = drag_origin;
	draw_selection(Bounds(position), color);

	if (selected_type != Type::Bridge)
	{
		constexpr float Extend = 0.15f;
		constexpr float Offset = 0.5f - Extend;

		Float2 origin = Float2(position) + Float2(0.5f);
		Float2 direction(selected_rotation.get_direction());
		draw_rectangle(origin + direction * Offset, Float2(Extend), Gate::ColorDisabled);
	}
}

void Cursor::DeviceTool::commit(Layer& layer)
{
	Int2 position = drag_origin;
	TileTag tile = layer.get(position);

	if (tile.type == TileType::Wire)
	{
		Wire::erase(layer, position);
		tile = TileTag();
	}

	if (selected_type == Type::Bridge)
	{
		if (tile.type != TileType::None) return;
		Bridge::insert(layer, position);
	}
	else
	{
		auto type = selected_type == Type::Transistor ? Gate::Type::Transistor : Gate::Type::Inverter;

		if (tile.type == TileType::Gate)
		{
			const Gate& gate = layer.get_list<Gate>()[tile.index];
			if (gate.get_type() == type && gate.get_rotation() == selected_rotation) return;
			Gate::erase(layer, position);
			tile = TileTag();
		}

		if (tile.type != TileType::None) return;
		Gate::insert(layer, position, type, selected_rotation);
	}
}

bool Cursor::RemovalTool::request_activation(const sf::Event& event)
{
	return event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Q;
}

void Cursor::RemovalTool::update(Int2 position)
{
	Bounds bounds(position);
	if (mouse_pressed()) bounds = Bounds::encapsulate(drag_origin, position);
	draw_removal(bounds);
}

void Cursor::RemovalTool::commit(Layer& layer)
{
	layer.erase(Bounds::encapsulate(drag_position, drag_origin));
}

Cursor::ClipboardTool::ClipboardTool(const Cursor& cursor) :
	Tool(cursor), layer_view(*cursor.layer_view),
	draw_context(std::make_unique<DrawContext>(cursor.application.get_shaders())) {}

void Cursor::ClipboardTool::update_interface()
{
	int type = static_cast<int>(selected_type);
	static constexpr std::array TypeNames = { "Cut", "Copy", "Paste" };
	ImGui::SliderInt("Clipboard Type", &type, 0, TypeNames.size() - 1, TypeNames[type]);
	selected_type = static_cast<Type>(type);

	imgui_tooltip(
		"Currently selected clipboard tool. Drag over area to highlight tiles to copy."
		"Can also be switched with button shortcuts: Cut = [X], Copy = [C], Paste = [V]"
	);

	if (buffer != nullptr)
	{
		if (selected_type == Type::Paste)
		{
			TileRotation rotation = buffer->get_rotation();

			int value = rotation.get_value();
			ImGui::SliderInt("Rotation", &value, 0, TileRotation::Count - 1, rotation.to_string());
			buffer->set_rotation(static_cast<TileRotation::Value>(value));

			imgui_tooltip("Rotation of the pasting orientation. Use [R] to quickly switch to the next rotation.");
		}

		Int2 size = buffer->size();
		ImGui::Text("Copied buffer: %ux%u", size.x, size.y);
	}
	else ImGui::TextUnformatted("No copied clipboard buffer");
}

bool Cursor::ClipboardTool::request_activation(const sf::Event& event)
{
	if (event.type != sf::Event::KeyPressed) return false;

	auto code = event.key.code;
	switch (code)
	{
		case sf::Keyboard::X:
		case sf::Keyboard::C:
		case sf::Keyboard::V:
		{
			if (code == sf::Keyboard::X) selected_type = Type::Cut;
			if (code == sf::Keyboard::C) selected_type = Type::Copy;
			if (code == sf::Keyboard::V) selected_type = Type::Paste;
			return true;
		}
		default: return false;
	}
}

void Cursor::ClipboardTool::input_event(const sf::Event& event)
{
	Tool::input_event(event);
	if (mouse_pressed()) return;

	if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::R) return;
	buffer->set_rotation(buffer->get_rotation().get_next());
}

Int2 Cursor::ClipboardTool::get_placement_size() const
{
	if (selected_type != Type::Paste) return Tool::get_placement_size();
	return buffer == nullptr ? Tool::get_placement_size() : buffer->size();
}

bool Cursor::ClipboardTool::restrict_drag_axis() const
{
	return selected_type == Type::Paste && buffer != nullptr;
}

void Cursor::ClipboardTool::update(Int2 position)
{
	if (selected_type == Type::Paste)
	{
		if (buffer == nullptr) return;

		static constexpr uint32_t ColorBackground = make_color(0, 0, 0);

		if (mouse_pressed())
		{
			Int2 min = drag_origin.min(position);
			Int2 max = drag_origin.max(position);
			Int2 size = buffer->size();

			draw_selection(Bounds(min, max + size), ColorBackground);

			(drag_type == DragType::Horizontal ? size.y : size.x) = 0;
			do buffer->draw(min, *draw_context, layer_view);
			while ((min += size) <= max);
		}
		else
		{
			Bounds bounds(position, position + buffer->size());
			draw_selection(bounds, ColorBackground);
			buffer->draw(position, *draw_context, layer_view);
		}
	}
	else
	{
		Bounds bounds(position);
		if (mouse_pressed()) bounds = Bounds::encapsulate(drag_origin, position);

		if (selected_type == Type::Cut) draw_removal(bounds);
		else draw_selection(bounds, make_color(210, 205, 220, 50));
	}
}

void Cursor::ClipboardTool::commit(Layer& layer)
{
	if (selected_type == Type::Paste)
	{
		if (buffer == nullptr) return;

		Int2 min = drag_origin.min(drag_position);
		Int2 max = drag_origin.max(drag_position);
		Int2 size = buffer->size();

		(drag_type == DragType::Horizontal ? size.y : size.x) = 0;
		do buffer->paste(min, layer);
		while ((min += size) <= max);
	}
	else
	{
		Bounds bounds = Bounds::encapsulate(drag_position, drag_origin);
		buffer = std::make_unique<Buffer>(layer, bounds);
		if (selected_type == Type::Cut) layer.erase(bounds);
	}
}

Cursor::ClipboardTool::Buffer::Buffer(const Layer& source, Bounds bounds) :
	buffer(std::make_unique<Layer>(source.copy(bounds))),
	bounds(bounds), rotation(TileRotation::Angle0) {}

void Cursor::ClipboardTool::Buffer::paste(Int2 position, Layer& layer) const
{
	//Precalculate transformation parameters
	Int2 one_less = size() - Int2(1);
	Int2 multiplier = Int2(1);

	if (rotation == TileRotation::Angle180 || rotation == TileRotation::Angle90)
	{
		multiplier.x = -1;
		position.x += one_less.x;
	}

	if (rotation == TileRotation::Angle180 || rotation == TileRotation::Angle270)
	{
		multiplier.y = -1;
		position.y += one_less.y;
	}

	//Insert new tiles to destination
	for (Int2 current : bounds)
	{
		//Skip empty tiles
		TileTag tile = buffer->get(current);
		if (tile.type == TileType::None) continue;

		//Transform current position based on rotation
		Int2 offset = current - bounds.get_min();
		if (rotation.vertical()) std::swap(offset.x, offset.y);
		current = offset * multiplier + position;

		if (not layer.has(current, TileType::None)) continue;

		//Insert tile
		switch (tile.type.get_value())
		{
			case TileType::Wire:
			{
				Wire::insert(layer, current);
				break;
			}
			case TileType::Bridge:
			{
				Bridge::insert(layer, current);
				break;
			}
			case TileType::Gate:
			{
				const Gate& gate = buffer->get_list<Gate>()[tile.index];
				Gate::insert(layer, current, gate.get_type(), rotation.rotate(gate.get_rotation()));

				break;
			}
			default: throw std::domain_error("Unrecognized TileType.");
		}
	}
}

void Cursor::ClipboardTool::Buffer::draw(Int2 position, DrawContext& context, const LayerView& layer_view) const
{
	Float2 offset = bounds.extend() + Float2(bounds.get_min());
	offset = rotation.rotate(offset);

	Float2 center = layer_view.get_center();
	center -= Float2(position) + Float2(size()) / 2.0f - offset;

	context.set_rotation(rotation);
	context.set_view(center, layer_view.get_extend());

	Float2 min(bounds.get_min());
	Float2 max(bounds.get_max());

	context.clip(min, max);
	buffer->draw(context, min, max);
	context.clear();
}

Debugger::Debugger(Application& application) : Component(application) {}

void Debugger::update()
{
	if (not imgui_begin("Debugger")) return;

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
		ImGui::LabelText("Tile Type", tile.type.to_string());

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
		const auto& states = layer_view->get_render_states();
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

TickControl::TickControl(Application& application) : Component(application) {}

void TickControl::initialize()
{
	controller = application.find_component<Controller>();
}

void TickControl::update()
{
	Layer* layer = controller->get_layer();

	if (imgui_begin("Ticks"))
	{
		if (layer != nullptr) update_interface();
		ImGui::End();
	}

	if (layer == nullptr) return;
	update(layer->get_engine());

	if (not selected_pause) last_display_time += Timer::as_float(application.get_timer().frame_time());
	if (last_display_time >= 1.0f) update_display();
}

void TickControl::input_event(const sf::Event& event)
{
	Component::input_event(event);
	if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::Space) return;
	if (selected_type == Type::Manual && remain_count == 0) begin_manual();
}

void TickControl::update_interface()
{
	ImGui::SeparatorText("Control");

	{
		static constexpr std::array Names = { "Per Second", "Per Frame", "Manual", "Maximum" };
		int old_type = static_cast<int>(selected_type);
		ImGui::Combo("Trigger Type", &old_type, Names.data(), Names.size());
		selected_type = static_cast<Type>(old_type);

		imgui_tooltip(
			"How update ticks are triggered. Per Second = ticks are triggered consistently across every second, "
			"Per Frame = ticks are triggered on every frame, Manual = ticks are triggered based on user input, "
			"Maximum = as many ticks as possible are triggered continuously before responsiveness is degraded"
		);

		if (selected_type != static_cast<Type>(old_type))
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
		uint32_t budget = std::chrono::duration_cast<std::chrono::milliseconds>(time_budget).count();
		ImGui::DragScalar("Time Budget", ImGuiDataType_U32, &budget, 1.0f, &TimeBudgetMin, &TimeBudgetMax, "%u ms");
		time_budget = as_duration(std::clamp(budget, TimeBudgetMin, TimeBudgetMax));
		imgui_tooltip("The time (in milliseconds) budgeted each frame for ticks; this may drastically affect responsiveness");
	}

	if (selected_type != Type::Maximum)
	{
		ImGui::DragScalar("Target Count", ImGuiDataType_U32, &selected_count);
		imgui_tooltip("The target (desired) number of ticks to trigger for");
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

		if (ImGui::Button("Begin")) begin_manual();
		imgui_tooltip("Manually trigger a target number of ticks. Can also activate with the [Space] button");

		ImGui::EndDisabled();
	}

	if (paused)
	{
		float x = ImGui::GetCursorPosX();
		ImGui::SameLine();
		ImGui::SetCursorPosX(x);
		ImGui::LabelText("Paused!", "");
	}

	ImGui::SeparatorText("Statistics");

	{
		ImGui::LabelText("Current FPS", display_frames_per_second.c_str());
		imgui_tooltip("Number of frames shown on screen every second.");
	}

	{
		std::string_view display = display_ticks_per_second;
		if (display_ticks_per_second.empty()) display = "0";
		ImGui::LabelText("Achieved TPS", display.data());
		imgui_tooltip("Currently achieved number of ticks per second");
	}

	if (not display_dropped_ticks.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
		ImGui::LabelText("Dropped Ticks", display_dropped_ticks.c_str());
		ImGui::PopStyleColor();

		imgui_tooltip(
			"Number of ticks targeted but unable to be achieved. This should hopefully be zero "
			"or otherwise Target Count is too high for this hardware and RedWire2 version."
		);
	}

	if (selected_type == Type::Manual && remain_count + executed.count > 0)
	{
		auto total = static_cast<float>(remain_count + executed.count);
		float progress = static_cast<float>(executed.count) / total;

		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::TextUnformatted("Tick Progress");

		imgui_tooltip("Completion progress of the manually triggered ticks");
	}
}

void TickControl::update_display()
{
	display_frames_per_second.clear();
	display_ticks_per_second.clear();
	display_dropped_ticks.clear();

	auto to_string = [](uint64_t value) { return std::to_string(value); };

	{
		uint64_t frame_count = application.get_timer().frame_count();
		uint64_t delta_frame_count = frame_count - last_frame_count;
		float rate = static_cast<float>(delta_frame_count) / last_display_time;

		display_frames_per_second = to_string(std::lround(rate));
		last_display_time = 0.0f;
		last_frame_count = frame_count;
	}

	if (executed.count > 0)
	{
		auto seconds = std::chrono::duration<float>(executed.duration);
		float ratio = static_cast<float>(executed.count) / seconds.count();
		display_ticks_per_second = to_string(static_cast<uint64_t>(ratio));

		if (selected_type != Type::Manual) executed = {};
	}

	if (dropped_count > 0)
	{
		display_dropped_ticks = to_string(dropped_count);
		dropped_count = 0;
	}
}

void TickControl::update(Engine& engine)
{
	if (selected_pause) return;

	switch (selected_type)
	{
		case Type::PerSecond:
		{
			float delta_time = Timer::as_float(application.get_timer().frame_time());
			float count = delta_time * static_cast<float>(selected_count);
			auto new_count = static_cast<uint64_t>(count);

			if (per_second_error >= 1.0f)
			{
				per_second_error -= 1.0f;
				++new_count;
			}

			remain_count = execute(engine, remain_count + new_count);
			per_second_error += count - static_cast<float>(new_count);

			if (remain_count > new_count)
			{
				uint64_t dropping = remain_count - new_count;
				dropped_count += dropping;
				remain_count -= dropping;
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
			execute(engine, std::numeric_limits<uint32_t>::max());
			break;
		}
	}
}

uint32_t TickControl::execute(Engine& engine, uint64_t count)
{
	assert(static_cast<uint32_t>(count) == count);
	if (count == 0) return 0;

	Duration budget = time_budget;
	TicksPair total;

	while (true)
	{
		auto attempt_budget = budget < as_duration(1) ? budget : budget / 2;
		uint64_t attempt = attempt_budget / last_execute_rate;
		attempt = std::clamp(attempt, uint64_t(1), count);

		auto start = Clock::now();
		engine.tick(static_cast<uint32_t>(attempt));

		Duration elapsed = Clock::now() - start;
		total += TicksPair(elapsed, attempt);
		count -= attempt;

		if (count == 0 || elapsed >= budget) break;
		budget -= elapsed;

		last_execute_rate = total.duration / total.count;
		last_execute_rate = std::max(last_execute_rate, Duration(1));
	}

	executed += total;
	return count;
}
}
