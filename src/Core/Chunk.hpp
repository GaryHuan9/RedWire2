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

	[[nodiscard]] bool empty() const { return tiles_count == 0; }

	[[nodiscard]] TileTag get(Int2 position) const { return tiles[get_index(position)]; }

	void set(Int2 position, TileTag tile)
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

	static constexpr int32_t SizeLog2 = 5;
	static constexpr int32_t Size = 1 << SizeLog2;

private:
	static int32_t get_index(Int2 position)
	{
		assert(0 <= position.x && position.x < Size);
		assert(0 <= position.y && position.y < Size);
		return position.y * Size + position.y;
	}

	size_t tiles_count = 0;
	std::unique_ptr<TileTag[]> tiles;
};

} // rw
