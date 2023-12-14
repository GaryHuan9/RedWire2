#include "Core/Tiles.hpp"
#include "Core/Board.hpp"

#include <random>

namespace rw
{

static const std::array<Int2, 4> four_directions = { Int2(1, 0), Int2(-1, 0), Int2(0, 1), Int2(0, -1) };

static uint32_t get_new_color()
{
	static std::uniform_int_distribution distribution;
	static std::default_random_engine random(42);
	return distribution(random) | 0xFFu;

}

void Wire::insert(Layer& layer, Int2 position)
{
	if (layer.get(position).type == TileType::Wire) return;

	auto& wires = layer.get_list<Wire>();

	size_t max_size = 0;
	uint32_t index = 0;

	for (Int2 direction : four_directions)
	{
		uint32_t data_index;
		if (not layer.try_get_index(position + direction, TileType::Wire, data_index)) continue;

		size_t chain_size = wires[data_index].chain.size();
		assert(chain_size > 0);

		if (max_size >= chain_size) continue;
		max_size = chain_size;
		index = data_index;
	}

	bool new_wire = max_size == 0;

	if (new_wire) index = wires.emplace(get_new_color());

	Chain& chain = wires[index].chain;
	layer.set(position, TileTag(TileType::Wire, index));
	chain.insert(position);

	if (new_wire) return;

	for (Int2 direction : four_directions)
	{
		uint32_t data_index;
		if (not layer.try_get_index(position + direction, TileType::Wire, data_index)) continue;
		if (data_index == index) continue;

		auto fill = [&](Int2 position)
		{
			layer.set(position, TileTag(TileType::Wire, index));
			chain.insert(position);
		};

		wires[data_index].chain.for_each(fill);
		wires.erase(data_index);
	}
}

void Wire::erase(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::None) return;
	assert(tile.type == TileType::Wire);

	auto& wires = layer.get_list<Wire>();
	Chain* chain = &wires[tile.index].chain;
	layer.set(position, TileTag());

	if (chain->size() == 1)
	{
		wires.erase(tile.index);
		return;
	}

	chain->erase(position);

	std::vector<Int2> neighbors;

	for (Int2 direction : four_directions)
	{
		Int2 neighbor = position + direction;
		if (not chain->contains(neighbor)) continue;
		neighbors.push_back(neighbor);
	}

	if (neighbors.size() == 1) return;

	bool first = true;

	while (not neighbors.empty())
	{
		Int2 neighbor = neighbors.back();
		neighbors.pop_back();

		std::vector<Int2> frontier;
		std::unordered_set<Int2> visited;

		frontier.push_back(neighbor);
		visited.insert(neighbor);

		uint32_t index = 0;
		Chain* new_chain = nullptr;

		if (not first)
		{
			index = wires.emplace(get_new_color());
			new_chain = &wires[index].chain;
			chain = &wires[tile.index].chain;
		}

		while (not frontier.empty())
		{
			Int2 local = frontier.back();
			frontier.pop_back();

			auto iterator = std::find(neighbors.begin(), neighbors.end(), local);

			if (iterator != neighbors.end())
			{
				neighbors.erase(iterator);
				if (first && neighbors.empty()) break;
			}

			if (not first)
			{
				layer.set(local, TileTag(TileType::Wire, index));
				new_chain->insert(local);
				chain->erase(local);
			}

			for (Int2 direction : four_directions)
			{
				Int2 offset = local + direction;
				if (not chain->contains(offset)) continue;
				if (not visited.insert(offset).second) continue;
				frontier.push_back(offset);
			}
		}

		first = false;
	}
}

}
