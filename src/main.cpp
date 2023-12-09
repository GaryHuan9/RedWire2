#include "main.hpp"
#include "Core/Layer.hpp"
#include "Interface/LayerView.hpp"

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

int main()
{
	sf::RenderWindow window(sf::VideoMode{ 1920, 1080 }, "RedWire2");
	window.setVerticalSyncEnabled(true);

	rw::Layer layer;
	rw::Float2 window_size(window.getSize());
	rw::LayerView layer_view(window_size.x / window_size.y);

	rw::Float2 last_mouse_percent;

	while (window.isOpen())
	{
		rw::Float2 mouse_percent = rw::Float2(sf::Mouse::getPosition(window)) / window_size;

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
					sf::Vector2f size(sf::Vector2u(event.size.width, event.size.height));
					window.setView(sf::View(size / 2.0f, size));
					layer_view.set_aspect_ratio(size.x / size.y);
					window_size = rw::Float2(size);

					break;
				}
				case sf::Event::MouseWheelScrolled:
				{
					layer_view.change_zoom(event.mouseWheelScroll.delta / -32.0f, mouse_percent);
					break;
				}
				default: break;
			}
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		{
			rw::Float2 point = layer_view.get_point(last_mouse_percent);
			layer_view.set_point(mouse_percent, point);
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
		{
			rw::Float2 point = layer_view.get_point(mouse_percent);
			layer.insert(rw::Float2::floor(point), rw::TileType::Wire);
		}

		layer_view.draw(window, layer);

		window.display();
		window.clear(sf::Color::Black);
		last_mouse_percent = mouse_percent;
	}

	return 0;
}
