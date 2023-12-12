#pragma once

#include "main.hpp"
#include "Chunk.hpp"
#include "Utility/Vector2.hpp"
#include "Utility/RecyclingList.hpp"

#include <array>
#include <tuple>
#include <unordered_map>

namespace rw
{

class Board
{
public:
	Board();

	template<class T>
	[[nodiscard]] const T& get_data(uint32_t index) const { return std::get<get_list_index<T>()>(data); }

	template<class T>
	T& get_data(uint32_t index) { return std::get<get_list_index<T>()>(data); }

private:
	template<class... Ts>
	using ListsType = std::tuple<RecyclingList<Ts>...>;

	template<class T, size_t I = 0>
	static constexpr size_t get_list_index()
	{
		if constexpr (std::is_same_v<std::tuple_element<I, decltype(data)>, T>) return I;
		else return get_list_index<T, I + 1>;
	};

	ListsType<Layer, Wire, Bridge> data;
};

class Layer
{
public:
	Layer();
	~Layer();

	void insert(Int2 position, TileType type);

	void erase(Int2 position);

	void draw(std::vector<sf::Vertex>& vertices, Float2 min, Float2 max, Float2 scale, Float2 origin) const;

	std::vector<Wire> wires;

private:
	[[nodiscard]] TileTag get(Int2 position);

	void set(Int2 position, TileTag tile);

	std::unordered_map<Int2, Chunk> chunks;
};

}
