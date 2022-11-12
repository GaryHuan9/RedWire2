#include "main.hpp"

#include <cmath>
#include "Vector2.hpp"
#include "Layer.hpp"

int main()
{
	sf::RenderWindow window(sf::VideoMode(800, 600), "Red Wire 2");

	rw::Layer layer;

	sf::Vector2i last_mouse_position = sf::Mouse::getPosition(window);
	rw::Float2 center;
	float zoom = -0.7f;

	while (window.isOpen())
	{
		sf::Event event{};

		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				case sf::Event::Closed:
				{
					window.close();
					break;
				}
				case sf::Event::Resized:
				{
					sf::Vector2u size(event.size.width, event.size.height);
					window.setView(sf::View(sf::Vector2f(size) / 2.0f, sf::Vector2f(size)));
					break;
				}
				case sf::Event::MouseWheelScrolled:
				{
					zoom += event.mouseWheelScroll.delta / 32.0f;
					break;
				}
				default: break;
			}
		}

		sf::Vector2i position = sf::Mouse::getPosition(window);
		sf::Vector2f delta(position - last_mouse_position);
		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) center -= rw::Float2(delta.x, delta.y) / zoom;
		last_mouse_position = position;

		window.clear(sf::Color::Black);

//		layer.draw(window, center, zoom);
		layer.draw(window, rw::Float2(), zoom);

		window.display();
	}

	return 0;
}