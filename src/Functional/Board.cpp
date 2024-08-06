#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"
#include "Functional/Engine.hpp"
#include "Functional/Drawing.hpp"

//#include "SFML/Graphics.hpp"

namespace rw
{

Board::Board() = default;

Layer::Layer() : lists(std::make_unique<ListsType>()), engine(std::make_unique<Engine>()) {}

Layer::~Layer() = default;

TileTag Layer::get(Int2 position) const
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);
	if (iterator == chunks.end()) return TileTag();
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
	auto draw = [&context, this](Chunk& chunk)
	{
		chunk.update_draw_buffer(context, *this);
		chunk.draw(context);
	};

	for_each_chunk(draw, Bounds(min_position, max_position));
}

void Layer::set(Int2 position, TileTag tile)
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);

	if (iterator == chunks.end())
	{
		if (tile.type == TileType::None) return;

		auto chunk = std::make_unique<Chunk>(chunk_position);
		auto pair = chunks.emplace(chunk_position, std::move(chunk));

		assert(pair.second);
		iterator = pair.first;
	}

	Chunk& chunk = *iterator->second;
	bool has_tiles = chunk.set(position, tile);
	if (not has_tiles) chunks.erase(iterator);
}

void Layer::erase(Bounds bounds)
{
	auto erase = [bounds, this](Chunk& chunk)
	{
		Int2 chunk_position = chunk.chunk_position * Chunk::Size;
		Bounds local_bounds = bounds - chunk_position;

		Int2 min = local_bounds.get_min().max(Int2(0));
		Int2 max = local_bounds.get_max().min(Int2(static_cast<int32_t>(Chunk::Size)));
		uint32_t remain = chunk.count();

		for (Int2 position : Bounds(min, max))
		{
			TileType type = chunk.get(position).type;
			if (type == TileType::None) continue;
			position += chunk_position;

			if (type == TileType::Wire) Wire::erase(*this, position);
			else if (type == TileType::Bridge) Bridge::erase(*this, position);
			else if (type == TileType::Gate) Gate::erase(*this, position);
			else throw std::domain_error("Unrecognized TileType.");

			//Since empty chunks are automatically removed, check to see when all tiles are removed to avoid accessing a bad chunk
			if (--remain == 0) break;
		}
	};

	for_each_chunk(erase, bounds);
}

Layer Layer::copy(Bounds bounds) const
{
	Layer layer;

	auto copy_chunk = [&layer](const Chunk& chunk)
	{
		auto pointer = std::make_unique<Chunk>(chunk);
		layer.chunks.emplace(chunk.chunk_position, std::move(pointer));
	};

	for_each_chunk(copy_chunk, bounds);
	layer.lists = std::make_unique<ListsType>(*lists);
	layer.engine = std::make_unique<Engine>(*engine);
	return layer;
}

BinaryWriter& operator<<(BinaryWriter& writer, const Layer& layer)
{
	writer << static_cast<uint32_t>(layer.chunks.size());

	for (const auto& [position, chunk] : layer.chunks)
	{
		writer << position;
		chunk->write(writer);
	}

	return writer << *layer.lists << *layer.engine;
}

BinaryReader& operator>>(BinaryReader& reader, Layer& layer)
{
	assert(layer.chunks.empty());

	uint32_t size;
	reader >> size;

	for (uint32_t i = 0; i < size; ++i)
	{
		Int2 position;
		reader >> position;

		auto pointer = std::make_unique<Layer::Chunk>(position);
		auto pair = layer.chunks.emplace(position, std::move(pointer));
		assert(pair.second);

		Layer::Chunk* chunk = pair.first->second.get();
		chunk->read(reader);
	}

	return reader >> *layer.lists >> *layer.engine;
}

template<class Action>
void Layer::for_each_chunk(Action action, Bounds bounds) const
{
	Bounds chunk_bounds = to_chunk_space(bounds);

	if (chunks.size() < chunk_bounds.size().product())
	{
		//Manual while loop to support removal of chunks during iteration
		auto iterator = chunks.begin();

		while (iterator != chunks.end())
		{
			Int2 chunk_position = iterator->first;
			Chunk* chunk = iterator->second.get();

			++iterator;
			if (chunk_bounds.contains(chunk_position)) action(*chunk);
		}
	}
	else
	{
		//Loop through all chunk positions
		for (Int2 position : chunk_bounds)
		{
			auto iterator = chunks.find(position);
			if (iterator == chunks.end()) continue;
			action(*iterator->second);
		}
	}
}

Bounds Layer::to_chunk_space(rw::Bounds bounds)
{
	return { Chunk::get_chunk_position(bounds.get_min()),
	         Chunk::get_chunk_position(bounds.get_max() - Int2(1)) + Int2(1) }; //Exclusive max
}

Layer::Chunk::Chunk(Int2 chunk_position) :
	chunk_position(chunk_position),
	tile_types(std::make_unique<decltype(tile_types)::element_type>()),
	tile_indices(std::make_unique<decltype(tile_indices)::element_type>()) {}

Layer::Chunk::Chunk(const Layer::Chunk& other) :
	chunk_position(other.chunk_position), occupied_tiles(other.occupied_tiles),
	tile_types(std::make_unique<decltype(tile_types)::element_type>(*other.tile_types)),
	tile_indices(std::make_unique<decltype(tile_indices)::element_type>(*other.tile_indices)),
	vertices_dirty(occupied_tiles > 0) {}

TileTag Layer::Chunk::get(Int2 position) const
{
	return get(get_tile_index(position));
}

uint32_t Layer::Chunk::count() const { return occupied_tiles; }

void Layer::Chunk::draw(DrawContext& context) const
{
	context.draw(true, vertex_buffer_quad);
	context.draw(false, vertex_buffer_wire);
}

bool Layer::Chunk::set(Int2 position, TileTag tile)
{
	uint32_t tile_index = get_tile_index(position);
	TileType& type = (*tile_types)[tile_index];
	uint32_t& index = (*tile_indices)[tile_index];

	if (type == tile.type && (type == TileType::None || index == tile.index)) return occupied_tiles > 0;

	if (type != TileType::None)
	{
		assert(occupied_tiles > 0);
		--occupied_tiles;
		vertices_dirty = true;
	}

	if (tile.type != TileType::None)
	{
		++occupied_tiles;
		index = tile.index;
		vertices_dirty = true;
	}

	type = tile.type;
	return occupied_tiles > 0;
}

void Layer::Chunk::update_draw_buffer(DrawContext& context, const Layer& layer)
{
	if (not vertices_dirty) return;
	vertices_dirty = false;

	for (Int2 position : Bounds(Int2(0), Int2(static_cast<int32_t>(Size))))
	{
		TileTag tile = get(position);
		position += chunk_position * Size;

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

	vertex_buffer_quad = context.flush_buffer(true);
	vertex_buffer_wire = context.flush_buffer(false);
}

void Layer::Chunk::write(BinaryWriter& writer) const
{
	TileTag last_tile;
	uint8_t count = 0;

	auto write = [&writer](TileTag tile, uint8_t count)
	{
		if (count == 0) return;
		writer << tile.type << count;

		if (tile.type == TileType::None) return;
		assert(tile.index.valid());
		writer << tile.index;
	};

	for (size_t i = 0; i < Size * Size; ++i)
	{
		TileTag tile = get(i);

		if (tile != last_tile)
		{
			write(last_tile, count);
			last_tile = tile;
			count = 1;
		}
		else if (++count == std::numeric_limits<decltype(count)>::max())
		{
			write(last_tile, count);
			count = 0;
		}
	}

	write(last_tile, count);
}

void Layer::Chunk::read(BinaryReader& reader)
{
	assert(occupied_tiles == 0);

	for (size_t i = 0; i < Size * Size;)
	{
		TileType type;
		uint8_t count;
		reader >> type >> count;

		assert(count != 0);
		size_t end = i + count;

		std::fill(tile_types->begin() + i, tile_types->begin() + end, type);

		if (type != TileType::None)
		{
			Index index;
			reader >> index;
			assert(index.valid());

			std::fill(tile_indices->begin() + i, tile_indices->begin() + end, index);
			occupied_tiles += count;
		}

		i = end;
	}

	if (occupied_tiles > 0) vertices_dirty = true;
}

TileTag Layer::Chunk::get(size_t tile_index) const
{
	TileType type = (*tile_types)[tile_index];
	if (type == TileType::None) return TileTag();
	Index index((*tile_indices)[tile_index]);
	return TileTag(type, index);
}

}
