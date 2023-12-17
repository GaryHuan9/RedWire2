#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

#include <SFML/Graphics.hpp>

namespace rw
{

Board::Board() : main_layer(std::make_unique<Layer>()) {}

Layer::Layer() = default;
Layer::~Layer() = default;

TileTag Layer::get(Int2 position) const
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);
	if (iterator == chunks.end()) return {};
	return iterator->second->get(position);
}

bool Layer::try_get_index(Int2 position, TileType type, Index& index) const
{
	TileTag tile = get(position);
	if (tile.type != type) return false;

	index = Index(tile.index);
	return true;
}

void Layer::set(Int2 position, TileTag tile)
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);

	if (iterator == chunks.end())
	{
		if (tile.type == TileType::None) return;
		auto pair = chunks.emplace(chunk_position, std::make_unique<Chunk>());

		assert(pair.second);
		iterator = pair.first;
	}

	Chunk& chunk = *iterator->second;
	chunk.set(position, tile);
	if (chunk.empty()) chunks.erase(iterator);
}

void Layer::draw(std::vector<sf::Vertex>& vertices, Float2 min, Float2 max, Float2 scale, Float2 origin) const
{
	Int2 chunk_min = Chunk::get_chunk_position(Float2::floor(min));
	Int2 chunk_max = Chunk::get_chunk_position(Float2::ceil(max));

	Int2 search_area = chunk_max - chunk_min;

	auto drawer = [&](Int2 chunk_position, const Chunk& chunk)
	{
		Float2 offset(chunk_position * static_cast<int32_t>(Chunk::Size));
		chunk.draw(vertices, *this, scale, offset * scale + origin);
	};

	if (chunks.size() < search_area.x * search_area.y)
	{
		for (const auto& pair : chunks)
		{
			Int2 chunk_position = pair.first;
			if (chunk_min.x <= chunk_position.x && chunk_position.x <= chunk_max.x &&
			    chunk_min.y <= chunk_position.y && chunk_position.y <= chunk_max.y)
				drawer(chunk_position, *pair.second);
		}
	}
	else
	{
		for (int32_t y = chunk_min.y; y <= chunk_max.y; ++y)
		{
			for (int32_t x = chunk_min.x; x <= chunk_max.x; ++x)
			{
				Int2 chunk_position(x, y);
				auto iterator = chunks.find(chunk_position);
				if (iterator == chunks.end()) continue;
				drawer(chunk_position, *iterator->second);
			}
		}
	}
}

TileTag Layer::Chunk::get(Int2 position) const
{
	size_t index = get_index(position);
	return { tile_types[index], Index(tile_indices[index]) };
}

void Layer::Chunk::set(Int2 position, TileTag tile)
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

void Layer::Chunk::draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const
{
	if (empty()) return;

	for (int32_t y = 0; y < Size; ++y)
	{
		for (int32_t x = 0; x < Size; ++x)
		{
			Int2 position(x, y);
			TileTag tile = get(position);
			uint32_t color = 0x050505FF;

			switch (tile.type)
			{
				case TileType::Wire:
				{
					color = layer.get_list<Wire>()[tile.index].color;
					break;
				}
				case TileType::Bridge:
				{
					color = 0x333333FF;
					break;
				}
				default: continue;
			}

			Float2 corner0 = Float2(position) * scale + origin;
			Float2 corner1 = corner0 + scale;

			vertices.emplace_back(sf::Vector2f(corner0.x, corner0.y), sf::Color(color));
			vertices.emplace_back(sf::Vector2f(corner1.x, corner0.y), sf::Color(color));
			vertices.emplace_back(sf::Vector2f(corner1.x, corner1.y), sf::Color(color));
			vertices.emplace_back(sf::Vector2f(corner0.x, corner1.y), sf::Color(color));

			if (tile.type != TileType::Wire) continue;


		}
	}
}

}
