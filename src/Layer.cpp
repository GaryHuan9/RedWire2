#include "Layer.hpp"

namespace rw
{

void Layer::draw(sf::RenderWindow& window, Float2 center, float scale)
{
	sf::Vector2f window_extend(window.getSize());
	window_extend /= 2.0f;

	sf::Vector2f view_center{ center.x, center.y };
	sf::Vector2f view_extend = window_extend / scale;

	sf::Vector2f view_min = view_center - view_extend;
	sf::Vector2f view_max = view_center + view_extend;

	sf::RectangleShape rectangle(sf::Vector2f(1.0f, window_extend.y * 2.0f));

	rectangle.setFillColor(sf::Color(255, 255, 255, 100));
	rectangle.setOrigin(rectangle.getSize() / 2.0f);

	auto unit = static_cast<int32_t>(std::round(std::log2(scale)));
	auto x_min = static_cast<int32_t>(std::floor(view_min.x / static_cast<float>(unit))) * unit;
	auto x_max = static_cast<int32_t>(std::ceil(view_max.x / static_cast<float>(unit))) * unit;

	for (int32_t x = x_min; x <= x_max; x += unit)
	{
		float window_x = (static_cast<float>(x) - view_center.x) * scale + window_extend.x;
		rectangle.setPosition(window_x, window_extend.y);
		window.draw(rectangle);
	}
}

} // rw2