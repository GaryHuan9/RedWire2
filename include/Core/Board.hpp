#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"
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

	[[nodiscard]] const Layer& get_layer() const { return *main_layer; }

	Layer& get_list() { return *main_layer; }

private:
	//	std::vector<Layer> layers; //TODO: add this in when we support layers
	std::unique_ptr<Layer> main_layer;
};

class Layer
{
public:
	Layer();
	~Layer();

	template<class T>
	[[nodiscard]] const RecyclingList<T>& get_list() const { return std::get<get_list_index<T>()>(lists); }

	template<class T>
	RecyclingList<T>& get_list() { return std::get<get_list_index<T>()>(lists); }

	[[nodiscard]] TileTag get(Int2 position) const;

	/**
	 * Tries to get the data index of the tile at `position`, if it is `type`.
	 * @param index The data index if returns true, undefined otherwise.
	 * @return True if the index was retrieved, false otherwise.
	 */
	[[nodiscard]] bool try_get_index(Int2 position, TileType type, Index& index) const;

	void set(Int2 position, TileTag tile);

	void draw(std::vector<sf::Vertex>& vertices, Float2 min, Float2 max, Float2 scale, Float2 origin) const;

private:
	class Chunk;

	template<class... Ts>
	using ListsType = std::tuple<RecyclingList<Ts>...>;

	template<class T, size_t I = 0>
	static constexpr size_t get_list_index()
	{
		if constexpr (std::is_same_v<typename std::tuple_element<I, decltype(lists)>::type, RecyclingList<T>>) return I;
		else return get_list_index<T, I + 1>();
	};

	std::unordered_map<Int2, std::unique_ptr<Chunk>> chunks;

	ListsType<Wire, Bridge> lists;
};

class Layer::Chunk
{
public:
	Chunk() : tile_types(new TileType[Size * Size]()), tile_indices(new uint32_t[Size * Size]()) {}

	[[nodiscard]] bool empty() const { return occupied_tiles == 0; }

	[[nodiscard]] TileTag get(Int2 position) const;

	void set(Int2 position, TileTag tile);

	void draw(std::vector<sf::Vertex>& vertices, const Layer& layer, Float2 scale, Float2 origin) const;

	static Int2 get_chunk_position(Int2 position) { return { position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2 }; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	static constexpr uint32_t SizeLog2 = 5;
	static constexpr uint32_t Size = 1u << SizeLog2;

private:
	static uint32_t get_index(Int2 position)
	{
		Int2 local_position = get_local_position(position);

		assert(0 <= local_position.x && local_position.x < Size);
		assert(0 <= local_position.y && local_position.y < Size);
		return local_position.y * Size + local_position.x;
	}

	size_t occupied_tiles = 0;
	std::unique_ptr<TileType[]> tile_types;
	std::unique_ptr<uint32_t[]> tile_indices;
};

}
