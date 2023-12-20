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

	static void draw(DrawContext& context, Int2 position, Index index, const Layer& layer);

#ifndef NDEBUG
	const uint32_t color;
#endif

private:
	/**
	 * Gets any wire neighbors of a position in certain directions.
	 * @return The position of the neighbors.
	 */
	static std::vector<Int2> get_neighbors(const Layer& layer, Int2 position, std::span<const Int2> directions);

	/**
	 * Gets any wire neighbors of a position in the four canonical directions, while using bridges.
	 * @return The position of the neighbors.
	 */
	static std::vector<Int2> get_neighbors_bridges(const Layer& layer, Int2 position);

	/**
	 * Gets neighbors from the same wire in the four canonical directions, while using and
	 * removing bridges that are disconnected because the wire at position has been removed.
	 * @return The position of the neighbors.
	 */
	static std::vector<Int2> fix_neighbors_bridges(Wire& wire, Int2 position);

	/**
	 * Merge all wires at positions into a single wire.
	 * @return Index of the final merged big wire.
	 */
	static Index merge_positions(Layer& layer, const std::vector<Int2>& positions);

	/**
	 * Splits all unconnected wires at positions into separate wires.
	 */
	static void split_positions(Layer& layer, std::vector<Int2>& positions);

	/**
	 * Splits a previously connected wire into separate wires at positions.
	 */
	static void split_positions(Layer& layer, std::vector<Int2>& positions, Index wire_index);

	std::unordered_set<Int2> positions;
	std::unordered_set<Int2> bridges;

	//The `bridges` set contains all bridges that are adjacent to `positions` in this wire
	//But the two sets are disjoint, so bridges are not contained within the `positions` set

	friend Bridge;
	friend Debugger;
};

class Bridge
{
public:
	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

	static void draw(DrawContext& context, Int2 position, Index index, const Layer& layer);

private:
};

class Gate
{
public:
	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

	static void draw(DrawContext& context, Int2 position, Index index, const Layer& layer);

private:
};

}
