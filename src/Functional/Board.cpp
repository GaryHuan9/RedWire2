#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"
#include "Functional/Engine.hpp"
#include "Functional/Drawing.hpp"

#include "SFML/Graphics.hpp"

namespace rw
{

Board::Board() = default;

Layer::Layer() : engine(std::make_unique<Engine>()) {}

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
	if (iterator == chunks.end()) return type == TileType::None;
	return iterator->second->get(position).type == type;
}

void Layer::draw(DrawContext& context, Float2 min_position, Float2 max_position) const
{
	auto draw = [&context](Chunk& chunk)
	{
		chunk.update_draw_buffer(context);
		chunk.draw(context);
	};

	for_each_chunk(draw, Float2::floor(min_position), Float2::ceil(max_position));
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

void Layer::erase(Int2 min_position, Int2 max_position)
{
	auto erase = [&](Chunk& chunk)
	{
		Int2 min = (min_position - chunk.chunk_position).max(Int2(0));
		Int2 max = (max_position - chunk.chunk_position).min(Int2(static_cast<int32_t>(Chunk::Size)));

		uint32_t remain = chunk.count();

		for (int32_t y = min.y; y < max.y; ++y)
		{
			for (int32_t x = min.x; x < max.x; ++x)
			{
				Int2 current(x, y);
				TileType type = chunk.get(current).type;
				if (type == TileType::None) continue;
				current += chunk.chunk_position;

				if (type == TileType::Wire) Wire::erase(*this, current);
				else if (type == TileType::Bridge) Bridge::erase(*this, current);
				else if (type == TileType::Gate) Gate::erase(*this, min_position);
				else throw std::domain_error("Unrecognized TileType.");

				//Since empty chunks are automatically removed, check to see when all tiles are removed to avoid accessing a bad chunk
				if (--remain == 0) return;
			}
		}
	};

	for_each_chunk(erase, min_position, max_position);
}

template<class Action>
void Layer::for_each_chunk(Action action, Int2 min_position, Int2 max_position) const
{
	Int2 min_chunk, max_chunk;
	get_chunk_bounds(min_position, max_position, min_chunk, max_chunk);

	if (chunks.size() < (max_chunk - min_chunk).product())
	{
		//Manual while loop to support removal of chunks during iteration
		auto iterator = chunks.begin();

		while (iterator != chunks.end())
		{
			Int2 chunk_position = iterator->first;
			Chunk* chunk = iterator->second.get();

			++iterator;
			if (min_chunk <= chunk_position && chunk_position < max_chunk) action(*chunk);
		}
	}
	else
	{
		//Loop through all chunk positions
		for (int32_t y = min_chunk.y; y < max_chunk.y; ++y)
		{
			for (int32_t x = min_chunk.x; x < max_chunk.x; ++x)
			{
				auto iterator = chunks.find(Int2(x, y));
				if (iterator == chunks.end()) continue;
				action(*iterator->second);
			}
		}
	}
}

void Layer::get_chunk_bounds(Int2 min_position, Int2 max_position, Int2& min_chunk, Int2& max_chunk)
{
	min_chunk = Chunk::get_chunk_position(min_position);
	max_chunk = Chunk::get_chunk_position(max_position - Int2(1)) + Int2(1); //Exclusive
}

Layer::Chunk::Chunk(const Layer& layer, Int2 chunk_position) :
	layer(layer), chunk_position(chunk_position),
	tile_types(new TileType[Size * Size]()),
	tile_indices(new uint32_t[Size * Size]()) {}

TileTag Layer::Chunk::get(Int2 position) const
{
	size_t index = get_index(position);
	TileType type = tile_types[index];
	if (type == TileType::None) return {};
	return { type, Index(tile_indices[index]) };
}

void Layer::Chunk::draw(DrawContext& context) const
{
	context.draw(true, vertex_buffer_quad);
	context.draw(false, vertex_buffer_wire);
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
		mark_dirty();
	}

	if (tile.type != TileType::None)
	{
		++occupied_tiles;
		tile_indices[index] = tile.index;
		mark_dirty();
	}

	type = tile.type;
	return occupied_tiles > 0;
}

void Layer::Chunk::update_draw_buffer(DrawContext& context)
{
	if (not vertices_dirty) return;
	vertices_dirty = false;

	for (int32_t y = 0; y < Size; ++y)
	{
		for (int32_t x = 0; x < Size; ++x)
		{
			Int2 position(x, y);
			TileTag tile = get(position);
			position += chunk_position;

			switch (tile.type.get_value())
			{
				case TileType::Wire:
				{
					Wire::draw(context, position, tile.index, layer);
					break;
				}
				case TileType::Bridge:
				{
					Bridge::draw(context, position, tile.index, layer);
					break;
				}
				case TileType::Gate:
				{
					Gate::draw(context, position, tile.index, layer);
					break;
				}
				case TileType::None: continue;
				default:
				{
					Float2 corner0(position);
					Float2 corner1 = corner0 + Float2(1.0f);
					context.emplace_quad(corner0, corner1, 0xFF00FFFF);
					break;
				}
			}
		}
	}

	vertex_buffer_quad = context.flush_buffer(true);
	vertex_buffer_wire = context.flush_buffer(false);
}

}
