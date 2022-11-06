#pragma once

#include "main.hpp"
#include "Vector2.hpp"

namespace rw
{

class Layer
{
public:
	void draw(sf::RenderWindow& window, Float2 center, float scale);
};

} // rw2
