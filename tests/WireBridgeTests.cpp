#include "main.hpp"
#include "Core/Board.hpp"
#include "Core/Tiles.hpp"

#include "gtest/gtest.h"

#include <string>
#include <random>
#include <unordered_set>

using namespace rw;

class WireBridgeTests : public testing::Test
{
protected:
	void SetUp() override
	{
		layer = std::make_unique<Layer>();
	}

	void insert_wires(Int2 position, Int2 gap = Int2(), uint32_t count = 1)
	{
		for (uint32_t i = 0; i < count; ++i, position += gap)
		{
			TileTag tile = layer->get(position);
			if (tile.type != TileType::None) continue;
			Wire::insert(*layer, position);
		}
	}

	void insert_bridge(Int2 position)
	{
		TileTag tile = layer->get(position);
		if (tile.type != TileType::None) return;
		Bridge::insert(*layer, position);
	}

	void erase(Int2 position, Int2 gap = Int2(), uint32_t count = 1)
	{
		for (uint32_t i = 0; i < count; ++i, position += gap)
		{
			TileTag tile = layer->get(position);
			if (tile.type == TileType::Wire) Wire::erase(*layer, position);
			else if (tile.type == TileType::Bridge) Bridge::erase(*layer, position);
		}
	}

	std::unique_ptr<Layer> layer;
};

TEST_F(WireBridgeTests, Simple)
{
	auto& wires = layer->get_list<Wire>();
	auto& bridges = layer->get_list<Bridge>();

	insert_wires(Int2(0, 0), Int2(1, 0), 10);
	insert_wires(Int2(0, 2), Int2(1, 0), 10);
	ASSERT_EQ(wires.size(), 2);

	insert_bridge(Int2(3, 1));
	ASSERT_EQ(wires.size(), 1);
	ASSERT_EQ(bridges.size(), 1);

	erase(Int2(3, 0));
	ASSERT_EQ(wires.size(), 3);
}

TEST_F(WireBridgeTests, DoubleBridge)
{
	auto& wires = layer->get_list<Wire>();
	auto& bridges = layer->get_list<Bridge>();

	insert_bridge(Int2(0, 1));
	insert_bridge(Int2(0, 8));
	ASSERT_EQ(bridges.size(), 2);

	insert_wires(Int2(0, 0), Int2(0, 1), 10);
	insert_wires(Int2(4, 0), Int2(0, 1), 10);
	insert_wires(Int2(-1, 1), Int2(1, 0), 7);
	insert_wires(Int2(-1, 8), Int2(1, 0), 7);
	ASSERT_EQ(wires.size(), 2);

	insert_wires(Int2(-1, 0));
	ASSERT_EQ(wires.size(), 1);

	insert_wires(Int2(-1, 9));
	ASSERT_EQ(wires.size(), 1);

	erase(Int2(0, 8));
	ASSERT_EQ(wires.size(), 2);

	erase(Int2(0, 1));
	ASSERT_EQ(wires.size(), 4);
	ASSERT_EQ(bridges.size(), 0);
}
