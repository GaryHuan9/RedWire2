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

LayerView::LayerView(Application& application) : Component(application), render_states(std::make_unique<sf::RenderStates>())
{
	Float2 window_size(window.getSize());
	set_aspect_ratio(window_size.x / window_size.y);

	auto shader_quad = std::make_unique<sf::Shader>();
	auto shader_wire = std::make_unique<sf::Shader>();

	shader_quad->loadFromFile("rsc/Tiles/Quad.vert", "rsc/Tiles/Tile.frag");
	shader_wire->loadFromFile("rsc/Tiles/Wire.vert", "rsc/Tiles/Tile.frag");
	draw_context = std::make_unique<DrawContext>(std::move(shader_quad), std::move(shader_wire));
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
	Float2 scale = Float2(1.0f) / extend;
	Float2 origin = -center * scale;
	draw_context->set_view(scale, origin);

	const void* states_data;
	size_t states_size;
	layer.get_engine().get_states(states_data, states_size);
	draw_context->set_wire_states(states_data, states_size);

	layer.draw(*draw_context, get_min(), get_max());
	draw_context->clear();
}

Cursor::Cursor(Application& application) : Component(application), rectangle(std::make_unique<sf::RectangleShape>()) {}

void Cursor::initialize()
{
	controller = application.find_component<Controller>();
	layer_view = application.find_component<LayerView>();
}

void Cursor::update()
{
	Layer* layer = controller->get_layer();
	Float2 mouse(sf::Mouse::getPosition(window));
	mouse_percent = mouse / Float2(window.getSize());
	mouse_percent.y = 1.0f - mouse_percent.y;

	last_mouse_point = mouse_point;
	recalculate_mouse_point();

	if (layer == nullptr)
	{
		if (imgui_begin("Cursor")) ImGui::End();
		return;
	}

	if (imgui_begin("Cursor"))
	{
		update_interface();
		ImGui::End();
	}

	if (application.handle_keyboard()) execute_key();
	if (Int2 position; try_get_mouse_position(position)) execute_mouse(position);
}

void Cursor::input_event(const sf::Event& event)
{
	Component::input_event(event);

	switch (event.type)
	{
		case sf::Event::MouseWheelScrolled:
		{
			float delta = event.mouseWheelScroll.delta / -32.0f;
			layer_view->change_zoom(delta, mouse_percent);
			break;
		}
		case sf::Event::KeyPressed:
		{
			execute_key_event(event);
			break;
		}
		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
		{
			execute_mouse_event(event);
			break;
		}
		default :break;
	}
}

bool Cursor::try_get_mouse_position(Int2& position) const
{
	if (not application.handle_mouse()) return false;
	position = Float2::floor(mouse_point);
	return true;
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
		selected_tool = static_cast<ToolType>(tool);

		imgui_tooltip(
			"Currently selected cursor tool. Can also be switched with button shortcuts: Mouse = [RMB], Wire = [E], "
			"Transistor = [Num1], Inverter = [Num2], Bridge = [Num3], Removal = [Q], Cut = [X], Copy = [C], Paste = [V]"
		);
	}

	switch (selected_tool)
	{
		case ToolType::WirePlacement:
		{
			ImGui::Checkbox("Auto Bridge Placement", &selected_auto_bridge);
			imgui_tooltip("Whether to automatically place bridges at wire junctions (i.e. tiles with more than two wire neighbors)");
			break;
		}
		case ToolType::PortPlacement:
		{
			static constexpr std::array PortNames = { "Transistor", "Inverter", "Bridge" };
			int port = static_cast<int>(selected_port);
			ImGui::SliderInt("Port", &port, 0, PortNames.size() - 1, PortNames[port]);
			selected_port = static_cast<PortType>(port);

			imgui_tooltip(
				"Currently selected port to place. Can also be switched with button shortcuts: "
				"Transistor = [Num1], Inverter = [Num2], Bridge = [Num3]"
			);

			if (selected_port == PortType::Bridge) break;

			int rotation = selected_rotation.get_value();
			ImGui::SliderInt("Rotation", &rotation, 0, 3, selected_rotation.to_string());
			selected_rotation = TileRotation(static_cast<TileRotation::Value>(rotation));
			imgui_tooltip("Rotation of the new port placed. Use [R] to quickly switch to the next rotation.");

			break;
		}
		case ToolType::Clipboard:
		{
			static constexpr std::array ClipNames = { "Cut", "Copy", "Paste" };
			int clip = static_cast<int>(selected_clip);
			ImGui::SliderInt("Clipboard Type", &clip, 0, ClipNames.size() - 1, ClipNames[clip]);
			selected_clip = static_cast<ClipType>(clip);

			imgui_tooltip(
				"Currently selected clipboard tool. Drag over area to highlight tiles to copy."
				"Can also be switched with button shortcuts: Cut = [X], Copy = [C], Paste = [V]"
			);

			if (selected_buffer != nullptr)
			{
				if (selected_clip == ClipType::Paste)
				{
					TileRotation rotation = selected_buffer->get_rotation();
					int value = rotation.get_value();
					ImGui::SliderInt("Rotation", &value, 0, 3, rotation.to_string());
					selected_buffer->set_rotation(static_cast<TileRotation::Value>(value));
					imgui_tooltip("Rotation of the pasting orientation. Use [R] to quickly switch to the next rotation.");
				}

				Int2 size = selected_buffer->size();
				ImGui::Text("Copied buffer: %ux%u", size.x, size.y);
			}
			else ImGui::TextUnformatted("No copied clipboard buffer");
		}
		default: break;
	}
}

void Cursor::execute_key()
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
	recalculate_mouse_point();
}

void Cursor::execute_mouse(Int2 position)
{
	bool button = sf::Mouse::isButtonPressed(sf::Mouse::Left);

	//Update mouse drag properties
	if (button)
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

	//Prepare for drawing
	static constexpr uint32_t OutlineColor = make_color(230, 225, 240);
	static constexpr uint32_t RemovalColor = make_color(220, 10, 30, 50);
	static constexpr uint32_t PastingColor = make_color(210, 205, 220, 50);

	Bounds draw_bounds(position);
	uint32_t fill_color = 0;

	//Execute selected tool
	switch (selected_tool)
	{
		case ToolType::Mouse:
		{
			if (button)
			{
				layer_view->set_point(mouse_percent, last_mouse_point);
				recalculate_mouse_point();
			}

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				Layer* layer = controller->get_layer();
				TileTag tile = layer->get(position);
				if (tile.type == TileType::Wire) layer->get_engine().toggle_wire_strong_powered(tile.index);
			}

			return;
		}
		case ToolType::WirePlacement:
		{
			if (button) draw_bounds = Bounds(place_wire(position));
			break;
		}
		case ToolType::PortPlacement:
		{
			if (button) draw_bounds = Bounds(place_port(position));

			if (selected_port != PortType::Bridge)
			{
				//Draw a little square to indicate rotation
				constexpr float Extend = 0.15f;
				constexpr float Offset = 0.5f - Extend;

				Float2 origin = draw_bounds.center();
				Int2 direction = selected_rotation.get_direction();
				Float2 center = origin + Float2(direction) * Offset;
				draw_rectangle(center, Float2(Extend * 2), OutlineColor);
			}

			break;
		}
		case ToolType::TileRemoval:
		{
			if (not button) break;
			assert(drag_type != DragType::None);

			draw_bounds = Bounds::encapsulate(drag_origin, position);
			fill_color = RemovalColor;
			break;
		}
		case ToolType::Clipboard:
		{
			if (selected_clip == ClipType::Paste)
			{
				if (selected_buffer == nullptr) return;

				Int2 min = selected_buffer->get_position(mouse_point);
				draw_bounds = { min, min + selected_buffer->size() };

				//TODO: draw clipboard content
			}
			else if (button)
			{
				assert(drag_type != DragType::None);

				draw_bounds = Bounds::encapsulate(drag_origin, position);
				fill_color = selected_clip == ClipType::Cut ? RemovalColor : PastingColor;
			}

			break;
		}
		default: break;
	}

	draw_rectangle(draw_bounds.center(), Float2(draw_bounds.size()), fill_color, 0.03f, OutlineColor);
}

void Cursor::execute_key_event(const sf::Event& event)
{
	assert(event.type == sf::Event::KeyPressed);
	sf::Keyboard::Key code = event.key.code;

	switch (event.key.code)
	{
		case sf::Keyboard::E:
		{
			selected_tool = ToolType::WirePlacement;
			break;
		}
		case sf::Keyboard::Q:
		{
			selected_tool = ToolType::TileRemoval;
			break;
		}
		case sf::Keyboard::R:
		{
			if (selected_tool == ToolType::PortPlacement && selected_port != PortType::Bridge) selected_rotation = selected_rotation.get_next();

			if (selected_tool == ToolType::Clipboard && selected_clip == ClipType::Paste && selected_buffer != nullptr)
			{
				TileRotation rotation = selected_buffer->get_rotation();
				selected_buffer->set_rotation(rotation.get_next());
			}

			break;
		}
		case sf::Keyboard::Num1:
		case sf::Keyboard::Num2:
		case sf::Keyboard::Num3:
		{
			selected_tool = ToolType::PortPlacement;

			if (code == sf::Keyboard::Num1) selected_port = PortType::Transistor;
			else if (code == sf::Keyboard::Num2) selected_port = PortType::Inverter;
			else if (code == sf::Keyboard::Num3) selected_port = PortType::Bridge;
			break;
		}
		case sf::Keyboard::X:
		case sf::Keyboard::C:
		case sf::Keyboard::V:
		{
			selected_tool = ToolType::Clipboard;

			if (code == sf::Keyboard::X) selected_clip = ClipType::Cut;
			else if (code == sf::Keyboard::C) selected_clip = ClipType::Copy;
			else if (code == sf::Keyboard::V) selected_clip = ClipType::Paste;
			break;
		}
		default: break;
	}
}

void Cursor::execute_mouse_event(const sf::Event& event)
{
	const auto& mouse = event.mouseButton;
	Layer* layer = controller->get_layer();

	switch (event.type)
	{
		case sf::Event::MouseButtonPressed:
		{
			if (mouse.button == sf::Mouse::Right) selected_tool = ToolType::Mouse;
			if (mouse.button != sf::Mouse::Left || layer == nullptr) break;

			if (selected_tool == ToolType::Clipboard && selected_clip == ClipType::Paste && selected_buffer != nullptr)
			{
				Int2 position = selected_buffer->get_position(mouse_point);
				layer->erase({ position, position + selected_buffer->size() });
				selected_buffer->paste(*layer, position);
			}

			break;
		}
		case sf::Event::MouseButtonReleased:
		{
			if (mouse.button != sf::Mouse::Left || drag_type == DragType::None) break;
			drag_type = DragType::None;

			Int2 position;
			if (layer == nullptr || not try_get_mouse_position(position)) break;

			Bounds bounds = Bounds::encapsulate(position, drag_origin);

			if (selected_tool == ToolType::TileRemoval) layer->erase(bounds);
			else if (selected_tool == ToolType::Clipboard && selected_clip != ClipType::Paste)
			{
				selected_buffer = std::make_unique<ClipBuffer>(*layer, bounds);
				if (selected_clip == ClipType::Cut) layer->erase(bounds);
			}

			break;
		}
		default: break;
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

	bool horizontal = drag_type == DragType::Horizontal;
	assert(horizontal || drag_type == DragType::Vertical);

	Int2 other_axis = horizontal ? Int2(0, 1) : Int2(1, 0);
	Int2 old_position = Float2::floor(last_mouse_point);
	auto drag = horizontal ?
	            std::make_pair(old_position.x, position.x) :
	            std::make_pair(old_position.y, position.y);

	if (drag.first > drag.second) std::swap(drag.first, drag.second);

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
				if (type == TileType::Wire) Wire::erase(layer, current);
				Bridge::insert(layer, current);
				continue;
			}
		}

		if (type == TileType::None) Wire::insert(layer, current);
	}

	return horizontal ? Int2(position.x, drag_origin.y) : Int2(drag_origin.x, position.y);
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

void Cursor::draw_rectangle(Float2 center, Float2 size, uint32_t fill_color, float outline, uint32_t outline_color)
{
	rectangle->setPosition(center.x - size.x / 2.0f, center.y - size.y / 2.0f);
	rectangle->setSize(sf::Vector2f(size.x, size.y));
	rectangle->setFillColor(sf::Color(fill_color));
	rectangle->setOutlineThickness(outline);
	rectangle->setOutlineColor(sf::Color(outline_color));
	window.draw(*rectangle, layer_view->get_render_states());
}

Cursor::ClipBuffer::ClipBuffer(const Layer& source, Bounds bounds) :
	source(std::make_unique<Layer>(source.copy(bounds))),
	bounds(bounds), rotation(TileRotation::East) {}

void Cursor::ClipBuffer::paste(Layer& layer, Int2 position) const
{
	//Precalculate transformation parameters
	Int2 one_less = size() - Int2(1);
	Int2 multiplier = Int2(1);

	if (rotation == TileRotation::West || rotation == TileRotation::North)
	{
		multiplier.x = -1;
		position.x += one_less.x;
	}

	if (rotation == TileRotation::West || rotation == TileRotation::South)
	{
		multiplier.y = -1;
		position.y += one_less.y;
	}

	//Insert new tiles to destination
	for (Int2 current : bounds)
	{
		//Skip empty tiles
		TileTag tile = source->get(current);
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
				const Gate& gate = source->get_list<Gate>()[tile.index];

				TileRotation gate_rotation = gate.get_rotation();
				gate_rotation = gate_rotation.rotate(TileRotation::East, rotation);
				Gate::insert(layer, current, gate.get_type(), gate_rotation);

				break;
			}
			default: throw std::domain_error("Unrecognized TileType.");
		}
	}
}

void Cursor::ClipBuffer::draw(DrawContext& context, Int2 position) const
{

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
		const char* display = display_ticks_per_second.c_str();
		if (display_ticks_per_second.empty()) display = "0";
		ImGui::LabelText("Achieved TPS", display);
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
