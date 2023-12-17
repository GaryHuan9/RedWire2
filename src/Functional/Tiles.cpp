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
	static std::default_random_engine random(42);
	std::uniform_int_distribution<uint32_t> distribution;
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
	Index wire_index = merge_positions(layer, neighbors);
	if (wire_index == Index()) wire_index = wires.emplace();

	//Assign tile position to wire
	auto& wire = wires[wire_index];
	wire.positions.insert(position);
	layer.set(position, TileTag(TileType::Wire, wire_index));

	//Add neighboring bridges to wire
	for (Int2 direction : FourDirections)
	{
		Int2 current = position + direction;
		if (layer.get(current).type != TileType::Bridge) continue;
		assert(not wire.positions.contains(current));
		wire.bridges.insert(current);
	}
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
	if (neighbors.size() > 1) split_positions(layer, neighbors, tile.index);
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

Index Wire::merge_positions(Layer& layer, const std::vector<Int2>& positions)
{
	if (positions.empty()) return {};

	//Find the longest neighboring wire
	auto& wires = layer.get_list<Wire>();
	Index wire_index;
	uint32_t max_length = 0;

	for (Int2 neighbor : positions)
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

	for (Int2 neighbor : positions)
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

void Wire::split_positions(Layer& layer, std::vector<Int2>& positions)
{
	std::vector<Int2> same_wire;

	while (not positions.empty())
	{
		Int2 position = positions.back();
		same_wire.push_back(position);
		positions.pop_back();

		Index wire_index;

		{
			TileTag tile = layer.get(position);
			assert(tile.type == TileType::Wire);
			wire_index = tile.index;
		}

		for (size_t i = 0; i < positions.size(); ++i)
		{
			Int2 current = positions[i];
			TileTag tile = layer.get(current);
			assert(tile.type == TileType::Wire);
			if (tile.index != wire_index) continue;

			same_wire.push_back(current);
			positions[i] = positions.back();
			positions.pop_back();
			--i;
		}

		split_positions(layer, same_wire, wire_index);
		assert(same_wire.empty());
	}
}

void Wire::split_positions(Layer& layer, std::vector<Int2>& positions, Index wire_index)
{
	for (Int2 neighbor : positions)
	{
		TileTag tile = layer.get(neighbor);
		assert(tile.type == TileType::Wire);
		assert(tile.index == wire_index);
	}

	if (positions.size() < 2)
	{
		positions.clear();
		return;
	}

	//The next procedure can create positions.size() - 1 new wires at a maximum
	//Must reserve to ensure addresses do not move when we emplace new wires
	auto& wires = layer.get_list<Wire>();
	wires.reserve(wires.size() + positions.size() - 1);
	Wire& wire = wires[wire_index]; //Do NOT move this above the previous line!

	//Initialize depth first search
	std::vector<Int2> frontier;
	std::unordered_set<Int2> visited;

	frontier.push_back(positions.back());
	visited.insert(positions.back());
	positions.pop_back();

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
			auto iterator = std::find(positions.begin(), positions.end(), next);
			if (iterator != positions.end() && (positions.erase(iterator), positions.empty())) return;

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
		Int2 neighbor = positions.back();
		positions.pop_back();

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
					assert(layer.has(next, TileType::Bridge));
					Int2 rotated = direction;
					std::swap(rotated.x, rotated.y);

					//If this bridge is still used by this wire in some way, we place it back to allow passing through it once again
					if (wire.positions.contains(next + rotated) || wire.positions.contains(next - rotated)) wire.bridges.insert(next);

					bridges.insert(next);
					next += direction;
				}

				if (not wire.positions.erase(next)) continue;
				bool inserted = visited.insert(next).second;
				assert(layer.has(next, TileType::Wire));
				assert(inserted);

				frontier.push_back(next);
			}
		}
		while (not frontier.empty());

		//Assign connected tiles to data
		std::erase_if(positions, [&visited](Int2 current) { return visited.contains(current); });
		std::swap(wires[new_index].positions, visited);
		std::swap(wires[new_index].bridges, bridges);

		assert(visited.empty());
		assert(bridges.empty());
	}
	while (not positions.empty());
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
		Index wire_index = Wire::merge_positions(layer, neighbors);
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

	auto neighbors = Wire::get_neighbors(layer, position, FourDirections, false);
	std::vector<Index> wire_indices;

	for (Int2 current : neighbors)
	{
		TileTag tile = layer.get(current);
		assert(tile.type == TileType::Wire);

		Wire& wire = layer.get_list<Wire>()[tile.index];
		assert(not wire.positions.contains(position));

		if (std::find(wire_indices.begin(), wire_indices.end(), tile.index) == wire_indices.end())
		{
			wire_indices.push_back(tile.index);
			bool erased = wire.bridges.erase(position);
			assert(erased);
		}

		assert(not wire.bridges.contains(position));
	}

	Wire::split_positions(layer, neighbors);
}

}
