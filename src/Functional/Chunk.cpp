#include "Core/Chunk.hpp"
#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

#include <SFML/Graphics.hpp>

namespace rw
{

TileTag Chunk::get(Int2 position) const
{
	size_t index = get_index(position);
	return { tile_types[index], tile_indices[index] };
}

void Chunk::set(Int2 position, TileTag tile)
{
	size_t index = get_index(position);
	TileType& type = tile_types[index];
	if (type != TileType::None) --occupied_tiles;

	if (tile.type == TileType::None)
	{
		type = TileType::None;
		return;
	}

	++occupied_tiles;
	type = tile.type;
	tile_indices[index] = tile.index;
}

void Chunk::draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const
{
	if (empty()) return;

	for (int32_t y = 0; y < Size; ++y)
	{
		for (int32_t x = 0; x < Size; ++x)
		{
			Int2 position(x, y);
			TileTag tile = get(position);
			if (tile.type == TileType::None) continue;

			Float2 corner0 = Float2(position) * scale + origin;
			Float2 corner1 = corner0 + scale;

			sf::Color color(layer.wires[tile.index].color);

			vertices.emplace_back(sf::Vector2f(corner0.x, corner0.y), color);
			vertices.emplace_back(sf::Vector2f(corner1.x, corner0.y), color);
			vertices.emplace_back(sf::Vector2f(corner1.x, corner1.y), color);
			vertices.emplace_back(sf::Vector2f(corner0.x, corner1.y), color);
		}
	}
}

}
