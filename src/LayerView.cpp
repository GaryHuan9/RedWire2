#include "LayerView.hpp"

namespace rw
{

void LayerView::draw(sf::RenderWindow& window) const
{
	auto window_size = Float2(window.getSize());

	vertices.clear();

	Float2 min = get_min();
	Float2 max = get_max();

	Float2 multiplier = window_size / 2.0f / extend;
	Float2 adder = window_size / 2.0f - center * multiplier;

	auto drawer = [&](int32_t gap, float percent)
	{
		Int2 int_min = Int2(
			static_cast<int32_t>(std::ceil(min.x / static_cast<float>(gap))),
			static_cast<int32_t>(std::ceil(min.y / static_cast<float>(gap)))
		) * gap;

		Int2 int_max = Int2(
			static_cast<int32_t>(std::floor(max.x / static_cast<float>(gap))),
			static_cast<int32_t>(std::floor(max.y / static_cast<float>(gap)))
		) * gap;

		sf::Color color(255, 255, 255, static_cast<int8_t>(GridLineAlpha * percent));
		if (color.a == 0) return;

		for (int32_t int_x = int_min.x; int_x <= int_max.x; int_x += gap)
		{
			float x = std::fma(static_cast<float>(int_x), multiplier.x, adder.x);

			vertices.emplace_back(sf::Vector2f(x, 0.0f), color);
			vertices.emplace_back(sf::Vector2f(x, window_size.y), color);
		}

		for (int32_t int_y = int_min.y; int_y <= int_max.y; int_y += gap)
		{
			float y = std::fma(static_cast<float>(int_y), multiplier.y, adder.y);

			vertices.emplace_back(sf::Vector2f(0.0f, y), color);
			vertices.emplace_back(sf::Vector2f(window_size.x, y), color);
		}
	};

	int32_t smaller_gap = zoom_gap / ZoomIncrement;

	if (smaller_gap > 0)
	{
		float alpha = std::sqrt(zoom_percent);
		drawer(smaller_gap, 1.0f - alpha);
		drawer(zoom_gap, alpha);
	}
	else drawer(zoom_gap, 1.0f);

	window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
}

void LayerView::update_zoom()
{
	zoom_gap = 1;
	zoom_level = static_cast<int32_t>(std::floor(zoom));
	for (int32_t i = 0; i < zoom_level; ++i) zoom_gap *= ZoomIncrement;

	zoom_scale = std::pow(static_cast<float>(ZoomIncrement), zoom);
	zoom_percent = zoom - static_cast<float>(zoom_level);
	extend = Float2(zoom_scale * aspect_ratio, zoom_scale);
}

} // rw2