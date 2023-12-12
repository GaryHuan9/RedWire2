#pragma once

#include "main.hpp"
#include "Utility/Vector2.hpp"

namespace rw
{

class Chunk
{
public:
	Chunk() : tile_types(new TileType[Size * Size]()), tile_indices(new uint32_t[Size * Size]()) {}

	[[nodiscard]] bool empty() const { return occupied_tiles == 0; }

	[[nodiscard]] TileTag get(Int2 position) const;

	void set(Int2 position, TileTag tile);

	void draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const;

	static Int2 get_chunk_position(Int2 position) { return { position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2 }; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	static constexpr uint32_t SizeLog2 = 5;
	static constexpr uint32_t Size = 1u << SizeLog2;

private:
	static uint32_t get_index(Int2 position)
	{
		Int2 local_position = get_local_position(position);

		assert(0 <= local_position.x && local_position.x < Size);
		assert(0 <= local_position.y && local_position.y < Size);
		return local_position.y * Size + local_position.x;
	}

	size_t occupied_tiles = 0;
	std::unique_ptr<TileType[]> tile_types;
	std::unique_ptr<uint32_t[]> tile_indices;
};

enum class TileType : uint8_t
{
	None,
	Wire,
	Bridge,
	Inverter,
	Transistor
};

class TileTag
{
public:
	TileTag() : TileTag(TileType::None, 0) {}

	TileTag(TileType type, uint32_t index) : type(type), index(index) {}

	const TileType type;
	const uint32_t index;
};

}
