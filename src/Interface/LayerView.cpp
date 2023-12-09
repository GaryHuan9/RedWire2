#include "Interface/LayerView.hpp"
#include "Core/Layer.hpp"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

namespace rw
{

LayerView::LayerView(float aspect_ratio, float zoom)
	: aspect_ratio(aspect_ratio), zoom(zoom) { update_zoom(); }

LayerView::~LayerView() = default;

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

	Float2 scale = window_size / 2.0f / extend;
	Float2 origin = window_size / 2.0f - center * scale;

	auto drawer = [&](int32_t gap, float percent)
	{
		Int2 int_min = Float2::ceil(min / static_cast<float>(gap)) * gap;
		Int2 int_max = Float2::floor(max / static_cast<float>(gap)) * gap;

		sf::Color color(255, 255, 255, static_cast<int8_t>(GridLineAlpha * percent));
		if (color.a == 0) return;

		for (int32_t int_x = int_min.x; int_x <= int_max.x; int_x += gap)
		{
			float x = std::fma(static_cast<float>(int_x), scale.x, origin.x);

			vertices.emplace_back(sf::Vector2f(x, 0.0f), color);
			vertices.emplace_back(sf::Vector2f(x, window_size.y), color);
		}

		for (int32_t int_y = int_min.y; int_y <= int_max.y; int_y += gap)
		{
			float y = std::fma(static_cast<float>(int_y), scale.x, origin.y);

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

} // rw
