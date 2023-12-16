#include "Core/Tiles.hpp"
#include "Core/Board.hpp"

#include <random>
#include <queue>

namespace rw
{

static constexpr std::array<Int2, 4> FourDirections = { Int2(1, 0), Int2(-1, 0), Int2(0, 1), Int2(0, -1) };

static uint32_t get_new_color()
{
	static std::uniform_int_distribution distribution;
	static std::default_random_engine random(42);
	return distribution(random) | 0xFFu;
}

bool Wire::get_longest_neighbor(const Layer& layer, Int2 position, Index& wire_index)
{
	const auto& wires = layer.get_list<Wire>();
	size_t max_length = 0;

	for (Int2 direction : FourDirections)
	{
		Index index;
		if (not layer.try_get_index(position + direction, TileType::Wire, index)) continue;

		size_t length = wires[index].length();
		assert(length > 0);

		if (max_length >= length) continue;
		max_length = length;
		wire_index = index;
	}

	return max_length > 0;
}

void Wire::merge_neighbors(Layer& layer, Int2 position, Index wire_index)
{
	auto& wires = layer.get_list<Wire>();
	auto& positions = wires[wire_index].positions;

	for (Int2 direction : FourDirections)
	{
		Index index;
		if (not layer.try_get_index(position + direction, TileType::Wire, index)) continue;
		if (index == wire_index) continue;

		for (Int2 current : wires[index].positions)
		{
			layer.set(current, TileTag(TileType::Wire, wire_index));
			positions.insert(current);
		}

		wires.erase(index);
	}
}

void Wire::insert(Layer& layer, Int2 position)
{
	if (layer.get(position).type == TileType::Wire) return;
	auto& wires = layer.get_list<Wire>();

	//Find longest neighbor wire
	Index wire_index;
	bool has_wire = get_longest_neighbor(layer, position, wire_index);
	if (not has_wire) wire_index = wires.emplace(get_new_color());

	//Add new position to wire
	auto& positions = wires[wire_index].positions;
	layer.set(position, TileTag(TileType::Wire, wire_index));
	positions.insert(position);

	//Merge all other neighbors together if needed
	if (has_wire) merge_neighbors(layer, position, wire_index);
}

bool Wire::erase_wire_position(Layer& layer, Int2 position, Index wire_index)
{
	auto& wires = layer.get_list<Wire>();
	auto& positions = wires[wire_index].positions;

	if (wires[wire_index].length() == 1)
	{
		assert(positions.contains(position));
		wires.erase(wire_index);
		return false;
	}

	bool erased = positions.erase(position);
	assert(erased);
	return true;
}

std::vector<Int2> Wire::get_neighbors(const Layer& layer, Int2 position, Index wire_index)
{
	std::vector<Int2> neighbors;
	auto& positions = layer.get_list<Wire>()[wire_index].positions;

	for (Int2 direction : FourDirections)
	{
		Int2 neighbor = position + direction;
		if (not positions.contains(neighbor)) continue;
		neighbors.push_back(neighbor);
	}

	return neighbors;
}

void Wire::split_neighbors(Layer& layer, std::vector<Int2>& neighbors, Index wire_index)
{
	//The next procedure can create neighbors.size() - 1 new wires at a maximum
	//Must reserve to ensure addresses do not move when we emplace new wires
	auto& wires = layer.get_list<Wire>();
	wires.reserve(wires.size() + neighbors.size() - 1);
	auto& positions = wires[wire_index].positions;

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
			if (not positions.contains(next)) continue;
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
	do
	{
		Index new_index = wires.emplace(get_new_color());
		Int2 neighbor = neighbors.back();
		neighbors.pop_back();

		frontier.push_back(neighbor);
		visited.insert(neighbor);
		bool erased = positions.erase(neighbor);
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
				if (not positions.erase(next)) continue;
				bool inserted = visited.insert(next).second;
				assert(inserted);

				frontier.push_back(next);
			}
		}
		while (not frontier.empty());

		//Assign connected tiles to data
		std::erase_if(neighbors, [&visited](Int2 current) { return visited.contains(current); });
		std::swap(wires[new_index].positions, visited);
		assert(visited.empty());
	}
	while (not neighbors.empty());
}

void Wire::erase(Layer& layer, Int2 position)
{
	TileTag wire = layer.get(position);
	if (wire.type == TileType::None) return;
	assert(wire.type == TileType::Wire);

	layer.set(position, TileTag());

	if (not erase_wire_position(layer, position, wire.index)) return;
	std::vector<Int2> neighbors = get_neighbors(layer, position, wire.index);
	if (neighbors.size() > 1) split_neighbors(layer, neighbors, wire.index);
}

}
