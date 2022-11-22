#pragma once

#include "main.hpp"
#include "TileTag.hpp"
#include "Utility/Vector2.hpp"

namespace rw
{

class Chunk
{
public:
	Chunk() : tiles(new TileTag[Size * Size]) {}

	[[nodiscard]]
	bool empty() const { return tiles_count == 0; }

	[[nodiscard]]
	TileTag get(Int2 position) const { return tiles[get_index(position)]; }

	void set(Int2 position, TileTag tile);

	void draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const;

	static Int2 get_chunk_position(Int2 position) { return { position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2 }; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	static const int32_t Size;

private:
	static int32_t get_index(Int2 position)
	{
		Int2 local_position = get_local_position(position);

		assert(0 <= local_position.x && local_position.x < Size);
		assert(0 <= local_position.y && local_position.y < Size);
		return local_position.y * Size + local_position.x;
	}

	size_t tiles_count = 0;
	std::unique_ptr<TileTag[]> tiles;

	static constexpr int32_t SizeLog2 = 5;
};

inline constexpr int32_t Chunk::Size = 1 << SizeLog2;

} // rw
