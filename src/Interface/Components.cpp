#include "Interface/Components.hpp"
#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

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

void LayerView::draw_grid(sf::RenderWindow& window) const
{
	auto window_size = Float2(window.getSize());

	Float2 min = get_min();
	Float2 max = get_max();

	float scale = window_size.x / 2.0f / extend.x; //This factor should be the same on both axes, using X here
	Float2 origin = window_size / 2.0f - center * scale;

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
	auto window_size = Float2(window.getSize());

	Float2 min = get_min();
	Float2 max = get_max();

	Float2 scale = window_size / (max - min);
	//	Float2 origin = (max + min) / -2.0f * scale;
	layer.draw(vertices, min, max, scale, -min * scale);

	window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Quads);
	vertices.clear();
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

	if (ImGui::Begin("Tools"))
	{
		int* pointer = reinterpret_cast<int*>(&selected_tool);
		ImGui::SliderInt("Selected", pointer, 0, Tools.size() - 1, Tools[selected_tool]);
	}

	ImGui::End();

	if (not Application::capture_mouse())
	{
		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) selected_tool = 0;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) selected_tool = 1;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) selected_tool = 2;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) selected_tool = 3;

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			Int2 position = Float2::floor(layer_view->get_point(mouse_percent));
			TileTag tile = layer->get(position);

			switch (selected_tool)
			{
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
}

void Controller::input_event(const sf::Event& event)
{
	Component::input_event(event);

	if (event.type == sf::Event::MouseWheelScrolled)
	{
		float delta = event.mouseWheelScroll.delta / -32.0f;
		layer_view->change_zoom(delta, mouse_percent);
	}
	else if (event.type == sf::Event::MouseMoved)
	{
		Float2 new_mouse_percent = Float2(event.mouseMove) / Float2(window.getSize());

		if (selected_tool == 0 && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			Float2 point = layer_view->get_point(mouse_percent);
			layer_view->set_point(new_mouse_percent, point);
		}

		mouse_percent = new_mouse_percent;
	}
}

}
