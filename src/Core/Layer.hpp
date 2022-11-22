#pragma once

#include <unordered_map>
#include "main.hpp"
#include "Chunk.hpp"
#include "TileType.hpp"
#include "Utility/Vector2.hpp"

namespace rw
{

class Layer
{
public:
	Layer();
	~Layer();

	void insert(Int2 position, TileType type);

	void erase(Int2 position);

	void draw(std::vector<sf::Vertex>& vertices, Float2 min, Float2 max, Float2 scale, Float2 origin) const;

	std::vector<WireData> wires;

private:
	[[nodiscard]]
	TileTag get(Int2 position);

	void set(Int2 position, TileTag tile);

	std::unordered_map<Int2, Chunk> chunks;
};

} // rw
