#include "Core/Tiles.hpp"
#include "Core/Board.hpp"

#include <random>
#include <utility>

namespace rw
{

static constexpr std::array<Int2, 4> FourDirections = { Int2(1, 0), Int2(-1, 0), Int2(0, 1), Int2(0, -1) };
static constexpr std::span<const Int2> HorizontalDirections(FourDirections.begin(), FourDirections.begin() + 2);
static constexpr std::span<const Int2> VerticalDirections(FourDirections.begin() + 2, FourDirections.end());

static uint32_t get_new_color()
{
	static std::uniform_int_distribution distribution;
	static std::default_random_engine random(42);
	return distribution(random) | 0xFFu;
}

Wire::Wire()
#ifndef NDEBUG
	: color(get_new_color())
#endif
{

}

void Wire::insert(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::Wire) return;
	assert(tile.type == TileType::None);

	//Find neighbors and merge them
	auto& wires = layer.get_list<Wire>();
	auto neighbors = get_neighbors(layer, position, FourDirections);
	Index wire_index = merge_neighbors(layer, neighbors);
	if (wire_index == Index()) wire_index = wires.emplace();

	//Assign tile position to wire
	layer.set(position, TileTag(TileType::Wire, wire_index));
	wires[wire_index].positions.insert(position);
}

void Wire::erase(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::None) return;
	assert(tile.type == TileType::Wire);

	//Remove tile position from wire
	auto& wires = layer.get_list<Wire>();
	Wire& wire = wires[tile.index];
	layer.set(position, TileTag());

	assert(not wire.bridges.contains(position));

	if (wire.length() == 1)
	{
		assert(wire.positions.contains(position));
		wires.erase(tile.index);
		return;
	}

	bool erased = wire.positions.erase(position);
	assert(erased);

	//Divide disconnected neighbors to different wires
	std::vector<Int2> neighbors = get_neighbors(layer, position, tile.index);
	if (neighbors.size() > 1) split_neighbors(layer, neighbors, tile.index);
}

std::vector<Int2> Wire::get_neighbors(const Layer& layer, Int2 position, std::span<const Int2> directions, bool use_bridge)
{
	std::vector<Int2> neighbors;

	//Finds all wires neighboring a tile
	for (Int2 direction : directions)
	{
		Int2 current = position + direction;
		TileTag tile = layer.get(current);

		//Forward position to next tile if is a bridge
		if (use_bridge && tile.type == TileType::Bridge)
		{
			current += direction;
			tile = layer.get(current);
		}

		if (tile.type == TileType::Wire) neighbors.push_back(current);
	}

	return neighbors;
}

std::vector<Int2> Wire::get_neighbors(const Layer& layer, Int2 position, Index wire_index)
{
	const Wire& wire = layer.get_list<Wire>()[wire_index];
	std::vector<Int2> neighbors;

	//Finds all positions neighboring a tile with the same wire index
	for (Int2 direction : FourDirections)
	{
		Int2 current = position + direction;
		if (wire.bridges.contains(current)) current += direction;
		if (not wire.positions.contains(current)) continue;
		neighbors.push_back(current);
	}

	return neighbors;
}

Index Wire::merge_neighbors(Layer& layer, const std::vector<Int2>& neighbors)
{
	if (neighbors.empty()) return {};

	//Find the longest neighboring wire
	auto& wires = layer.get_list<Wire>();
	Index wire_index;
	uint32_t max_length = 0;

	for (Int2 neighbor : neighbors)
	{
		TileTag tile = layer.get(neighbor);
		assert(tile.type == TileType::Wire);

		uint32_t length = wires[tile.index].length();
		assert(length > 0);

		if (max_length >= length) continue;
		wire_index = tile.index;
		max_length = length;
	}

	//Merge other wires together
	Wire& wire = wires[wire_index];

	for (Int2 neighbor : neighbors)
	{
		TileTag tile = layer.get(neighbor);
		assert(tile.type == TileType::Wire);
		if (tile.index == wire_index) continue;

		const Wire& old_wire = wires[tile.index];
		for (Int2 current : old_wire.positions) layer.set(current, TileTag(TileType::Wire, wire_index));

		//Union the two sets
		wire.positions.insert(old_wire.positions.begin(), old_wire.positions.end());
		wire.bridges.insert(old_wire.bridges.begin(), old_wire.bridges.end());
		wires.erase(tile.index);
	}

	return wire_index;
}

void Wire::split_neighbors(Layer& layer, std::vector<Int2>& neighbors, Index wire_index)
{
	for (Int2 neighbor : neighbors)
	{
		TileTag tile = layer.get(neighbor);
		assert(tile.type == TileType::Wire);
		assert(tile.index == wire_index);
	}

	if (neighbors.size() < 2) return;

	//The next procedure can create neighbors.size() - 1 new wires at a maximum
	//Must reserve to ensure addresses do not move when we emplace new wires
	auto& wires = layer.get_list<Wire>();
	Wire& wire = wires[wire_index];
	wires.reserve(wires.size() + neighbors.size() - 1);

	//Initialize depth first search
	std::vector<Int2> frontier;
	std::unordered_set<Int2> visited;

	frontier.push_back(neighbors.back());
	visited.insert(neighbors.back());
	neighbors.pop_back();

	//Perform first search, tries to reach all neighbors
	do
	{
		Int2 current = frontier.back();
		frontier.pop_back();

		for (Int2 direction : FourDirections)
		{
			Int2 next = current + direction;
			if (wire.bridges.contains(next)) next += direction;
			if (not wire.positions.contains(next)) continue;
			if (not visited.insert(next).second) continue;

			//Immediately stop search if all neighbors are connected
			auto iterator = std::find(neighbors.begin(), neighbors.end(), next);
			if (iterator != neighbors.end() && (neighbors.erase(iterator), neighbors.empty())) return;

			frontier.push_back(next);
		}
	}
	while (not frontier.empty());

	visited.clear();

	//The same wire no longer connects all positions, need to create new wires
	std::unordered_set<Int2> bridges;

	do
	{
		Index new_index = wires.emplace();
		Int2 neighbor = neighbors.back();
		neighbors.pop_back();

		frontier.push_back(neighbor);
		visited.insert(neighbor);
		bool erased = wire.positions.erase(neighbor);
		assert(erased);

		//Perform full search to change all connected tiles to this new wire
		do
		{
			Int2 current = frontier.back();
			frontier.pop_back();

			layer.set(current, TileTag(TileType::Wire, new_index));

			for (Int2 direction : FourDirections)
			{
				Int2 next = current + direction;

				if (wire.bridges.erase(next))
				{
					if (bridges.insert(next).second)
					{
						Int2 rotated = direction;
						std::swap(rotated.x, rotated.y);

						//If this bridge is used twice for this wire, we have to allow checking once more by placing it back
						bool used_twice = wire.positions.contains(next + rotated) || wire.positions.contains(next - rotated);
						if (used_twice) wire.bridges.insert(next);
					}

					next += direction;
				}

				if (not wire.positions.erase(next)) continue;
				bool inserted = visited.insert(next).second;
				assert(inserted);

				frontier.push_back(next);
			}
		}
		while (not frontier.empty());

		//Assign connected tiles to data
		std::erase_if(neighbors, [&visited](Int2 current) { return visited.contains(current); });
		std::swap(wires[new_index].positions, visited);
		std::swap(wires[new_index].bridges, bridges);
		assert(visited.empty());
	}
	while (not neighbors.empty());
}

void Bridge::insert(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::Bridge) return;
	assert(tile.type == TileType::None);

	auto& bridges = layer.get_list<Bridge>();
	layer.set(position, TileTag(TileType::Bridge, bridges.emplace()));

	auto update_wires = [&layer, position](std::span<const Int2> directions)
	{
		auto neighbors = Wire::get_neighbors(layer, position, directions, false);
		Index wire_index = Wire::merge_neighbors(layer, neighbors);
		if (wire_index == Index()) return;

		Wire& wire = layer.get_list<Wire>()[wire_index];
		assert(not wire.positions.contains(position));
		wire.bridges.insert(position);
	};

	update_wires(HorizontalDirections);
	update_wires(VerticalDirections);
}

void Bridge::erase(Layer& layer, Int2 position)
{
	TileTag bridge = layer.get(position);
	if (bridge.type == TileType::None) return;
	assert(bridge.type == TileType::Bridge);

	auto& bridges = layer.get_list<Bridge>();
	bridges.erase(bridge.index);
	layer.set(position, TileTag());

	auto update_wires = [&layer, position](std::span<const Int2> directions)
	{
		std::vector<Int2> neighbors = Wire::get_neighbors(layer, position, directions, false);
		Index wire_index;

		for (Int2 neighbor : neighbors)
		{
			TileTag tile = layer.get(neighbor);
			assert(tile.type == TileType::Wire);

			Wire& wire = layer.get_list<Wire>()[tile.index];
			assert(not wire.positions.contains(position));

			//May skip the splitting if the wire has already been divided from the updating in the other axis
			if (wire_index != Index() && wire_index != tile.index)
			{
				assert(not wire.bridges.contains(position));
				return;
			}

			wire.bridges.erase(position);
			wire_index = tile.index;
		}

		Wire::split_neighbors(layer, neighbors, wire_index);
	};

	update_wires(HorizontalDirections);
	update_wires(VerticalDirections);
}

}
