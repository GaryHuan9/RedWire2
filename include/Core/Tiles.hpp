#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"

#include <unordered_set>
#include <span>

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

struct TileTag
{
	TileTag() : TileTag(TileType::None, Index()) {}

	TileTag(TileType type, Index index) : type(type), index(index) {}

	TileType type;
	Index index;
};

class Wire
{
public:
	Wire();

	[[nodiscard]] uint32_t length() const { return positions.size(); }

	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

#ifndef NDEBUG
	const uint32_t color;
#endif

private:
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, std::span<const Int2> directions, bool use_bridge = true);
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, Index wire_index);

	static Index merge_neighbors(Layer& layer, const std::vector<Int2>& neighbors);
	static void split_neighbors(Layer& layer, std::vector<Int2>& neighbors, Index wire_index);

	std::unordered_set<Int2> positions;
	std::unordered_set<Int2> bridges;

	//The `bridges` set contains all bridges that are adjacent to `positions` in this wire
	//But the two sets are disjoint, so bridges are not contained within the `positions` set

	friend Bridge;
};

class Bridge
{
public:
	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);
private:
};

}
