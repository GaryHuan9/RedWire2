#pragma once

#include "main.hpp"
#include "TileType.hpp"

namespace rw
{

class TileTag
{
public:
	TileTag() : TileTag(TileType::None, 0) {}

	TileTag(TileType type, uint32_t index) : type(type), index(index) {}

public:
	TileType type;
	uint32_t index;
};

} // rw
