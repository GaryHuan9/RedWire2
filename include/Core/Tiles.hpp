#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"

#include <unordered_set>
#include <span>

namespace rw
{

class TileType
{
public:
	enum Value : uint8_t
	{
		None,
		Wire,
		Bridge,
		Gate,
		Note
	};

	constexpr TileType(Value value = Value::None) : value(value) {} // NOLINT(*-explicit-constructor)

	[[nodiscard]] Value get_switch() const { return value; }

	[[nodiscard]] const char* to_string() const;

	[[nodiscard]] constexpr bool operator==(TileType other) const { return value == other.value; }

	[[nodiscard]] constexpr bool operator!=(TileType other) const { return value != other.value; }

private:
	Value value;
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

	static void draw(const Layer& layer, Index index);

#ifndef NDEBUG
	const uint32_t color;
#endif

private:
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, std::span<const Int2> directions, bool use_bridge = true);
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, Index wire_index);

	static Index merge_positions(Layer& layer, const std::vector<Int2>& positions);
	static void split_positions(Layer& layer, std::vector<Int2>& positions);
	static void split_positions(Layer& layer, std::vector<Int2>& positions, Index wire_index);

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

class Gate
{
public:
	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

private:
};

}
