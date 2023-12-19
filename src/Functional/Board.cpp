#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

#include <SFML/Graphics.hpp>

namespace rw
{

Board::Board() = default;

Layer::Layer() = default;
Layer::~Layer() = default;

TileTag Layer::get(Int2 position) const
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);
	if (iterator == chunks.end()) return {};
	return iterator->second->get(position);
}

bool Layer::has(Int2 position, TileType type) const
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);
	if (iterator == chunks.end()) return false;
	return iterator->second->get(position).type == type;
}

void Layer::set(Int2 position, TileTag tile)
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);

	if (iterator == chunks.end())
	{
		if (tile.type == TileType::None) return;

		auto chunk = std::make_unique<Chunk>(*this, chunk_position * Chunk::Size);
		auto pair = chunks.emplace(chunk_position, std::move(chunk));

		assert(pair.second);
		iterator = pair.first;
	}

	Chunk& chunk = *iterator->second;
	bool has_tiles = chunk.set(position, tile);
	if (not has_tiles) chunks.erase(iterator);
}

void Layer::draw(DrawContext& context, Float2 min_position, Float2 max_position) const
{
	Int2 min_chunk = Chunk::get_chunk_position(Float2::floor(min_position));
	Int2 max_chunk = Chunk::get_chunk_position(Float2::ceil(max_position) - Int2(1)); //max_chunk is inclusive
	Int2 search_area = min_chunk - max_chunk;

	if (chunks.size() < search_area.x * search_area.y)
	{
		for (const auto& pair : chunks)
		{
			Int2 chunk_position = pair.first;
			if (min_chunk.x > chunk_position.x || chunk_position.x > max_chunk.x) continue;
			if (min_chunk.y > chunk_position.y || chunk_position.y > max_chunk.y) continue;
			pair.second->draw(context);
		}
	}
	else
	{
		for (int32_t y = min_chunk.y; y <= max_chunk.y; ++y)
		{
			for (int32_t x = min_chunk.x; x <= max_chunk.x; ++x)
			{
				auto iterator = chunks.find(Int2(x, y));
				if (iterator == chunks.end()) continue;
				iterator->second->draw(context);
			}
		}
	}
}

Layer::Chunk::Chunk(const Layer& layer, Int2 chunk_position) :
	layer(layer), chunk_position(chunk_position),
	tile_types(new TileType[Size * Size]()),
	tile_indices(new uint32_t[Size * Size]()),
	vertices_wire(std::make_unique<sf::VertexBuffer>(sf::PrimitiveType::Quads, sf::VertexBuffer::Usage::Dynamic)),
	vertices_static(std::make_unique<sf::VertexBuffer>(sf::PrimitiveType::Quads, sf::VertexBuffer::Usage::Dynamic)) {}

TileTag Layer::Chunk::get(Int2 position) const
{
	size_t index = get_index(position);
	TileType type = tile_types[index];
	if (type == TileType::None) return {};
	return { type, Index(tile_indices[index]) };
}

bool Layer::Chunk::set(Int2 position, TileTag tile)
{
	uint32_t index = get_index(position);
	TileType& type = tile_types[index];

	if (type == tile.type && (type == TileType::None || tile_indices[index] == tile.index)) return occupied_tiles > 0;

	if (type != TileType::None)
	{
		assert(occupied_tiles > 0);
		--occupied_tiles;
		vertices_dirty = true;
	}

	if (tile.type != TileType::None)
	{
		++occupied_tiles;
		vertices_dirty = true;
		tile_indices[index] = tile.index;
	}

	type = tile.type;
	return true;
}

void Layer::Chunk::draw(DrawContext& context) const
{
	if (vertices_dirty)
	{
		update_vertices();
		vertices_dirty = false;
	}

	context.window.draw(*vertices_wire, context.states_wire);
	context.window.draw(*vertices_static, context.states_static);
}

void Layer::Chunk::update_vertices() const
{
	std::vector<sf::Vertex> wires;
	std::vector<sf::Vertex> statics;

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

			auto corner0 = Float2(position + chunk_position);
			Float2 corner1 = corner0 + Float2(1.0f);

			auto& buffer = (tile.type == TileType::Wire ? wires : statics);
			buffer.emplace_back(sf::Vector2f(corner0.x, corner0.y), sf::Color(color));
			buffer.emplace_back(sf::Vector2f(corner1.x, corner0.y), sf::Color(color));
			buffer.emplace_back(sf::Vector2f(corner1.x, corner1.y), sf::Color(color));
			buffer.emplace_back(sf::Vector2f(corner0.x, corner1.y), sf::Color(color));
		}
	}

	vertices_wire->create(wires.size());
	vertices_wire->update(wires.data());
	vertices_static->create(statics.size());
	vertices_static->update(statics.data());
}

DrawContext::DrawContext(sf::RenderWindow& window, const sf::RenderStates& states_wire, const sf::RenderStates& states_static) :
	window(window), states_wire(states_wire), states_static(states_static) {}

}
