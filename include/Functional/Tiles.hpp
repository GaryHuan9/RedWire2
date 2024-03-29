#pragma once

#include "main.hpp"
#include "Utility/SimpleTypes.hpp"
#include "Utility/Functions.hpp"

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

	constexpr TileType(Value value = Value::None) : data(value) // NOLINT(*-explicit-constructor)
	{
		assert(static_cast<unsigned int>(value) < Count);
	}

	[[nodiscard]] constexpr Value get_value() const { return data; }

	[[nodiscard]] const char* to_string() const;

	[[nodiscard]] constexpr bool operator==(TileType other) const { return get_value() == other.get_value(); }

	[[nodiscard]] constexpr bool operator!=(TileType other) const { return get_value() != other.get_value(); }

	friend BinaryWriter& operator<<(BinaryWriter& writer, TileType type)
	{
		return writer << type.get_value();
	}

	friend BinaryReader& operator>>(BinaryReader& reader, TileType& type)
	{
		TileType::Value value;
		reader >> value;
		type = value;
		return reader;
	}

	static constexpr uint32_t Count = 5;

private:
	Value data;
};

class TileRotation
{
public:
	enum Value : uint8_t
	{
		Angle0,
		Angle90,
		Angle180,
		Angle270
	};

	constexpr TileRotation(Value value = Value::Angle0) : data(value) // NOLINT(*-explicit-constructor)
	{
		assert(static_cast<unsigned int>(value) < Count);
	}

	[[nodiscard]] bool vertical() const { return data == Value::Angle90 || data == Value::Angle270; }

	/**
	 * Gets the next rotation in clockwise order.
	 */
	[[nodiscard]]
	TileRotation get_next() const
	{
		uint32_t next = get_value() - 1;
		return { static_cast<Value>(next % Count) };
	}

	/**
	 * Rotates another TileRotation.
	 */
	[[nodiscard]] TileRotation rotate(TileRotation value) const
	{
		uint32_t rotated = get_value() + value.get_value();
		return { static_cast<Value>(rotated % Count) };
	}

	/**
	 * Rotates a Vector2<T>.
	 */
	template<class T>
	Vector2<T> rotate(Vector2<T> value) const
	{
		switch (data)
		{
			case 0: return value;
			case 1: return Vector2<T>(-value.y, value.x);
			case 2: return Vector2<T>(-value.x, -value.y);
			case 3: return Vector2<T>(value.y, -value.x);
			default: throw std::domain_error("Bad rotation.");
		}
	}

	[[nodiscard]] Int2 get_direction() const;

	[[nodiscard]] constexpr Value get_value() const { return data; }

	[[nodiscard]] const char* to_string() const;

	[[nodiscard]] constexpr bool operator==(TileRotation other) const { return get_value() == other.get_value(); }

	[[nodiscard]] constexpr bool operator!=(TileRotation other) const { return get_value() != other.get_value(); }

	friend BinaryWriter& operator<<(BinaryWriter& writer, TileRotation rotation)
	{
		return writer << rotation.get_value();
	}

	friend BinaryReader& operator>>(BinaryReader& reader, TileRotation& rotation)
	{
		TileRotation::Value value;
		reader >> value;
		rotation = value;
		return reader;
	}

	static constexpr uint32_t Count = 4;

private:
	Value data;
};

struct TileTag
{
	explicit TileTag(TileType type = TileType::None, Index index = Index()) : type(type), index(index) {}

	bool operator==(const TileTag& other) const { return type == other.type && index == other.index; }

	bool operator!=(const TileTag& other) const { return not(other == *this); }

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
	 * Updates all four gates neighboring a wire at position.
	 */
	static void update_neighbors_gates(Layer& layer, Int2 position);

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

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Wire& wire);
	friend BinaryReader& operator>>(BinaryReader& reader, Wire& wire);

#ifndef NDEBUG
	const uint32_t color;
#endif

	static constexpr uint32_t ColorUnpowered = make_color(71, 0, 22);
	static constexpr uint32_t ColorPowered = make_color(254, 22, 59);
	static constexpr uint32_t ColorStrong = make_color(247, 137, 27);

private:
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

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Bridge& bridge) { return writer; }

	friend BinaryReader& operator>>(BinaryReader& reader, Bridge& bridge) { return reader; }

	static constexpr uint32_t Color = make_color(182, 52, 62);
};

class Gate
{
public:
	enum class Type : uint8_t
	{
		Transistor,
		Inverter
	};

	Gate() : Gate(Type::Transistor, TileRotation()) {}

	Gate(Type type, TileRotation rotation);

	[[nodiscard]] Type get_type() const { return type; }

	[[nodiscard]] TileRotation get_rotation() const { return rotation; }

	static void insert(Layer& layer, Int2 position, Gate::Type type, TileRotation rotation);

	static void erase(Layer& layer, Int2 position);

	static void draw(DrawContext& context, Int2 position, Index index, const Layer& layer);

	static void update(Layer& layer, Int2 position);

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Gate& gate)
	{
		writer << gate.type << gate.rotation;
		for (Index index : gate.wire_indices) writer << index;
		return writer;
	}

	friend BinaryReader& operator>>(BinaryReader& reader, Gate& gate)
	{
		reader >> gate.type >> gate.rotation;
		for (Index& index : gate.wire_indices) reader >> index;
		return reader;
	}

	static constexpr uint32_t ColorTransistor = make_color(62, 173, 95);
	static constexpr uint32_t ColorInverter = make_color(59, 73, 255);
	static constexpr uint32_t ColorDisabled = make_color(18, 17, 24);

private:
	[[nodiscard]] Index output_index() const { return wire_indices.front(); }

	[[nodiscard]] Index& output_index() { return wire_indices.front(); }

	[[nodiscard]] std::span<const Index> input_indices() const { return { wire_indices.begin() + 1, wire_indices.end() }; }

	[[nodiscard]] std::span<Index> input_indices() { return { wire_indices.begin() + 1, wire_indices.end() }; }

	Type type;
	TileRotation rotation;
	std::array<Index, 4> wire_indices;

	friend Debugger;
};

}
