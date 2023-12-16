#pragma once

#include "main.hpp"
#include "Utility/Vector2.hpp"
#include <unordered_set>

namespace rw
{

enum class TileType : uint8_t
{
	None,
	Wire,
	Bridge,
	Note,
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

class Wire
{
public:
	explicit Wire(uint32_t color) : color(color) {}

	[[nodiscard]] uint32_t length() const { return positions.size(); }

	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

	uint32_t color;

private:
	static bool get_longest_neighbor(const Layer& layer, Int2 position, uint32_t& wire_index);
	static void merge_neighbors(Layer& layer, Int2 position, uint32_t wire_index);

	static bool erase_wire_position(Layer& layer, Int2 position, uint32_t wire_index);
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, uint32_t wire_index);
	static void split_neighbors(Layer& layer, std::vector<Int2>& neighbors, uint32_t wire_index);

	std::unordered_set<Int2> positions;
};

class Bridge
{
public:
private:
};

}
