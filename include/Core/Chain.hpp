#pragma once

#include "main.hpp"
#include "Utility/Vector2.hpp"
#include <unordered_set>

namespace rw
{

class Chain
{
public:
	bool contains(Int2 position) const;

	bool insert(Int2 position);
	bool erase(Int2 position);

private:
	class Segment;

	std::unordered_set<Int2> positions;
	//	std::vector<Segment> segments; //TODO: simply using unordered_set for now, should change to the segment system for performance
};

class Chain::Segment //A segment of chunks that densely store a chunk of positions
{
public:
	bool contains(Int2 position) const;

private:
	static Int2 get_offset(uint8_t delta);

	Int2 head_position;
	Int2 tail_position;
	std::vector<uint64_t> chunks;
	std::vector<uint8_t> offsets; //We can actually pack this even further: delta only needs four directions (2 bits)
};

} // rw
