#include "Functional/Tiles.hpp"
#include "Functional/Board.hpp"
#include "Functional/Engine.hpp"
#include "Drawing.hpp"

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

template<class T>
static auto set_intersect(const std::unordered_set<T>& set0, const std::unordered_set<T>& set1)
{
	auto* search = &set0;
	auto* check = &set1;
	if (search->size() > check->size()) std::swap(search, check);

	std::unordered_set<T> result;

	for (const T& value : *search)
	{
		if (check->contains(value)) result.insert(value);
	}

	return result;
}

const char* TileType::to_string() const
{
	static constexpr std::array<const char*, 5> Strings = { "None", "Wire", "Bridge", "Gate", "Note" };
	return Strings[get_value()];
}

TileRotation TileRotation::get_next() const
{
	static constexpr std::array<TileRotation, 4> NextValues = { TileRotation::South, TileRotation::North,
	                                                            TileRotation::West, TileRotation::East };
	return NextValues[get_value()];
}

Int2 TileRotation::get_direction() const
{
	return FourDirections[get_value()];
}

const char* TileRotation::to_string() const
{
	static constexpr std::array<const char*, 5> Strings = { "East", "West", "South", "North" };
	return Strings[get_value()];
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
	auto neighbors = get_neighbors_bridges(layer, position);
	Index wire_index = merge_positions(layer, neighbors);
	if (wire_index == Index()) wire_index = wires.emplace();

	//Assign tile position to wire
	auto& wire = wires[wire_index];
	wire.positions.insert(position);
	layer.set(position, TileTag(TileType::Wire, wire_index));
	layer.get_engine().register_wire(wire_index);
	update_neighbors_gates(layer, position);

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

	//Remove tile position from wire and layer
	auto& wires = layer.get_list<Wire>();
	Wire& wire = wires[tile.index];
	layer.set(position, TileTag());
	update_neighbors_gates(layer, position);

	assert(not wire.bridges.contains(position));
	assert(wire.positions.contains(position));

	if (wire.length() == 1)
	{
		wires.erase(tile.index);
		return;
	}

	bool erased = wire.positions.erase(position);
	assert(erased);

	//Divide disconnected neighbors to different wires
	std::vector<Int2> neighbors = fix_neighbors_bridges(wire, position);
	if (neighbors.size() > 1) split_positions(layer, neighbors, tile.index);
}

void Wire::draw(DrawContext& context, Int2 position, Index index, const Layer& layer)
{
	const auto& wire = layer.get_list<Wire>()[index];

	auto corner0 = Float2(position);
	Float2 corner1 = corner0 + Float2(1.0f);
	context.emplace(true, corner0, corner1, wire.color);
}

std::vector<Int2> Wire::get_neighbors(const Layer& layer, Int2 position, std::span<const Int2> directions)
{
	std::vector<Int2> neighbors;

	for (Int2 direction : directions)
	{
		Int2 current = position + direction;
		TileTag tile = layer.get(current);

		if (tile.type == TileType::Wire) neighbors.push_back(current);
	}

	return neighbors;
}

std::vector<Int2> Wire::get_neighbors_bridges(const Layer& layer, Int2 position)
{
	std::vector<Int2> neighbors;

	for (Int2 direction : FourDirections)
	{
		Int2 current = position + direction;
		TileTag tile = layer.get(current);

		//Forward position to next tile if is a bridge
		if (tile.type == TileType::Bridge)
		{
			current += direction;
			tile = layer.get(current);
		}

		if (tile.type == TileType::Wire) neighbors.push_back(current);
	}

	return neighbors;
}

std::vector<Int2> Wire::fix_neighbors_bridges(Wire& wire, Int2 position)
{
	assert(not wire.positions.contains(position));

	std::vector<Int2> neighbors;

	for (Int2 direction : FourDirections)
	{
		Int2 current = position + direction;

		if (not wire.positions.contains(current))
		{
			if (not wire.bridges.contains(current)) continue;

			if (not wire.positions.contains(current + direction))
			{
				Int2 rotated = direction;
				std::swap(rotated.x, rotated.y);

				//Erase bridge from wire if all four of its direct neighbors do not exist anymore (i.e. position, current + direction, +- rotated)
				if (not wire.positions.contains(current + rotated) && not wire.positions.contains(current - rotated)) wire.bridges.erase(current);
				continue;
			}

			current += direction;
		}

		neighbors.push_back(current);
	}

	return neighbors;
}

void Wire::update_neighbors_gates(Layer& layer, Int2 position)
{
	assert(layer.has(position, TileType::Wire));

	for (Int2 direction : FourDirections)
	{
		Int2 current = position + direction;
		if (layer.has(current, TileType::Gate)) Gate::update(layer, current);
	}
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

	for (Int2 position : positions)
	{
		TileTag tile = layer.get(position);
		assert(tile.type == TileType::Wire);
		if (tile.index == wire_index) continue;
		const Wire& old_wire = wires[tile.index];

		for (Int2 current : old_wire.positions)
		{
			layer.set(current, TileTag(TileType::Wire, wire_index));
			update_neighbors_gates(layer, current);
		}

		//Union the two sets
		wire.positions.insert(old_wire.positions.begin(), old_wire.positions.end());
		wire.bridges.insert(old_wire.bridges.begin(), old_wire.bridges.end());
		assert(set_intersect(wire.positions, wire.bridges).empty());

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
	for (Int2 position : positions)
	{
		TileTag tile = layer.get(position);
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

		{
			Int2 current = positions.back();
			positions.pop_back();

			frontier.push_back(current);
			visited.insert(current);
			bool erased = wire.positions.erase(current);
			assert(erased);
		}

		//Perform full search to change all connected tiles to this new wire
		do
		{
			Int2 current = frontier.back();
			frontier.pop_back();

			layer.set(current, TileTag(TileType::Wire, new_index));
			update_neighbors_gates(layer, current);

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

					assert(not visited.contains(next));
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

		assert(set_intersect(visited, bridges).empty());
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
		auto neighbors = Wire::get_neighbors(layer, position, directions);
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
	TileTag tile = layer.get(position);
	if (tile.type == TileType::None) return;
	assert(tile.type == TileType::Bridge);

	auto& bridges = layer.get_list<Bridge>();
	bridges.erase(tile.index);
	layer.set(position, TileTag());

	auto neighbors = Wire::get_neighbors(layer, position, FourDirections);
	std::vector<Index> wire_indices;

	for (Int2 current : neighbors)
	{
		TileTag neighbor = layer.get(current);
		assert(neighbor.type == TileType::Wire);

		Wire& wire = layer.get_list<Wire>()[neighbor.index];
		assert(not wire.positions.contains(position));

		if (std::find(wire_indices.begin(), wire_indices.end(), neighbor.index) == wire_indices.end())
		{
			wire_indices.push_back(neighbor.index);
			bool erased = wire.bridges.erase(position);
			assert(erased);
		}

		assert(not wire.bridges.contains(position));
	}

	Wire::split_positions(layer, neighbors);
}

void Bridge::draw(DrawContext& context, Int2 position, Index index, const Layer& layer)
{
	//	const auto& bridges = layer.get_list<Bridge>()[index];

	auto corner0 = Float2(position);
	Float2 corner1 = corner0 + Float2(1.0f);
	context.emplace(false, corner0, corner1, 0xB6343EFF);
}

Gate::Gate(Gate::Type type, const TileRotation& rotation) : type(type), rotation(rotation)
{
	assert(type == Type::Transistor || type == Type::Inverter);
	assert(rotation.get_next() == rotation.get_next()); //Assert rotation is valid
}

void Gate::insert(Layer& layer, Int2 position, Gate::Type type, TileRotation rotation)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::Gate) return;
	assert(tile.type == TileType::None);

	auto& gates = layer.get_list<Gate>();
	Index gate_index = gates.emplace(type, rotation);
	layer.set(position, TileTag(TileType::Gate, gate_index));

	update(layer, position);
}

void Gate::erase(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	if (tile.type == TileType::None) return;
	assert(tile.type == TileType::Gate);

	auto& gates = layer.get_list<Gate>();
	gates.erase(tile.index);
	layer.set(position, TileTag());
}

void Gate::draw(DrawContext& context, Int2 position, Index index, const Layer& layer)
{
	static constexpr uint32_t ColorTransistor = 0x3EAD5FFF;
	static constexpr uint32_t ColorInverter = 0x3B49FFFF;
	static constexpr uint32_t ColorDisabled = 0x121118FF;
	static constexpr float DisabledSize = 0.5f;

	const auto& gate = layer.get_list<Gate>()[index];
	uint32_t color = gate.type == Type::Transistor ? ColorTransistor : ColorInverter;

	auto corner0 = Float2(position);
	Float2 corner1 = corner0 + Float2(1.0f);
	context.emplace(false, corner0, corner1, color);

	Int2 direction = gate.rotation.get_direction();
	Float2 origin = corner0 + Float2(0.5f);
	Float2 center = origin + Float2(direction) * (0.5f - DisabledSize / 2.0f);
	corner0 = center - Float2(DisabledSize / 2.0f);
	corner1 = corner0 + Float2(DisabledSize);

	context.emplace(false, corner0, corner1, 0x111118FF);
}

void Gate::update(Layer& layer, Int2 position)
{
	TileTag tile = layer.get(position);
	assert(tile.type == TileType::Gate);
	auto& gate = layer.get_list<Gate>()[tile.index];

	TileRotation rotation = gate.rotation;

	for (Index& wire_index : gate.wire_indices)
	{
		TileTag neighbor = layer.get(position + rotation.get_direction());
		wire_index = neighbor.type == TileType::Wire ? neighbor.index : Index();
		rotation = rotation.get_next();
	}
}

}
