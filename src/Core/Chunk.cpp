#include "Chunk.hpp"
#include "Layer.hpp"
#include "WireData.hpp"

#include <SFML/Graphics.hpp>

namespace rw
{

void Chunk::set(Int2 position, TileTag tile)
{
	TileTag& reference = tiles[get_index(position)];
	if (reference.type != TileType::None) --tiles_count;

	if (tile.type != TileType::None)
	{
		++tiles_count;
		reference = tile;
	}
	else reference = TileTag();
}

void Chunk::draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const
{
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

} // rw
