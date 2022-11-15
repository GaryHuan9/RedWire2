#pragma once

#include <unordered_map>
#include "main.hpp"
#include "Chunk.hpp"

namespace rw
{

class Layer
{
public:
	[[nodiscard]]
	TileTag get(Int2 position)
	{
		Chunk& chunk = get_chunk(position);
		return chunk.get(get_local_position(position));
	}

	void set(Int2 position, TileTag tile)
	{
		Chunk& chunk = get_chunk(position);
		chunk.set(get_local_position(position), tile);
	}

private:
	Chunk& get_chunk(Int2 position) { return chunks[Int2(position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2)]; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	std::unordered_map<Int2, Chunk> chunks;
};

} // rw
