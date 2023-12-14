#include "Core/Chain.hpp"

namespace rw
{

static constexpr uint32_t ChunkSizeLog2 = 3;
static constexpr uint32_t ChunkSize = 1 << ChunkSizeLog2;

static Int2 get_chunk_position(Int2 position) { return { position.x >> ChunkSizeLog2, position.y >> ChunkSizeLog2 }; }

static Int2 get_local_position(Int2 position) { return { position.x & (ChunkSize - 1), position.y & (ChunkSize - 1) }; }

static uint32_t get_local_index(Int2 local_position)
{
	assert(0 <= local_position.x && local_position.x < ChunkSize);
	assert(0 <= local_position.y && local_position.y < ChunkSize);
	return local_position.x + local_position.y * ChunkSize;
}

bool Chain::contains(Int2 position) const
{
	return positions.contains(position);
}

bool Chain::insert(Int2 position)
{
	return positions.insert(position).second;
}

bool Chain::erase(Int2 position)
{
	return positions.erase(position) > 0;
}

//bool Chain::Segment::contains(Int2 position) const
//{
//	Int2 current = head_position;
//
//	Int2 chunk_position = get_chunk_position(position);
//	Int2 local_position = get_local_position(position);
//	uint32_t local_index = get_local_index(local_position);
//
//	for (size_t i = 0; i < offsets.size(); ++i)
//	{
//		if (current == chunk_position)
//		{
//			uint64_t chunk = chunks[i];
//			uint64_t bit = (chunk >> local_index) & 1;
//			return bit == 1;
//		}
//
//		current = get_offset(offsets[i]);
//	}
//
//	return false;
//}
//
//Int2 Chain::Segment::get_offset(uint8_t packed)
//{
//	switch (packed)
//	{
//		case 0: return { 1, 0 };
//		case 1: return { -1, 0 };
//		case 2: return { 0, 1 };
//		case 3: return { 0, -1 };
//		default: throw std::domain_error("Bad packing.");
//	}
//}

} // rw
