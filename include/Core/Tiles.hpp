#pragma once

#include "main.hpp"
#include "Chain.hpp"

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
	//public:
	//	[[nodiscard]]
	//	bool get() const { return value != 0; }
	//
	//	void set() { value ^= 1; }
	//
	//	void
	//
	//private:
	//	uint8_t value = false;

public:
	explicit Wire(uint32_t color) : color(color) {}

	static void insert(Layer& layer, Int2 position);

	static void erase(Layer& layer, Int2 position);

	uint32_t color;

private:
	Chain chain;
};

class Bridge
{
public:
private:
};

}
