#include "main.hpp"
#include "LayerView.hpp"

int main()
{
	sf::RenderWindow window(sf::VideoMode{ 1920, 1080 }, "RedWire2");

	rw::LayerView layer(static_cast<float>(window.getSize().x) / static_cast<float>(window.getSize().y));

	sf::Vector2i last_mouse_position = sf::Mouse::getPosition(window);

	while (window.isOpen())
	{
		sf::Event event{};
		rw::Float2 mouse_delta;

		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				case sf::Event::Closed:
				{
					window.close();
					break;
				}
				case sf::Event::MouseMoved:
				{
					mouse_delta += rw::Float2(rw::Int2(event.mouseMove.x, event.mouseMove.y));
					break;
				}
				case sf::Event::Resized:
				{
					sf::Vector2f size(sf::Vector2u(event.size.width, event.size.height));
					window.setView(sf::View(size / 2.0f, size));
					layer.set_aspect_ratio(size.x / size.y);

					break;
				}
				case sf::Event::MouseWheelScrolled:
				{
					layer.change_zoom(event.mouseWheelScroll.delta / 32.0f);
					break;
				}
				default: break;
			}
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) std::cout << mouse_delta << std::endl;

		layer.draw(window);

		window.display();
		window.clear(sf::Color::Black);
	}

	return 0;
}