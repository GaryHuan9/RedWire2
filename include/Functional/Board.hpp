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

	[[nodiscard]] const Layer& get_layer(Index index) const { return layers[index]; }

	Layer& get_layer(Index index) { return layers[index]; }

private:
	std::vector<Layer> layers;
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

	[[nodiscard]] Engine& get_engine() const { return *engine; }

	[[nodiscard]] TileTag get(Int2 position) const;

	[[nodiscard]] bool has(Int2 position, TileType type) const;

	void set(Int2 position, TileTag tile);

	void draw(DrawContext& context, Float2 min_position, Float2 max_position) const;

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

	ListsType<Wire, Bridge, Gate> lists;
	std::unique_ptr<Engine> engine;
};

class Layer::Chunk
{
public:
	Chunk(const Layer& layer, Int2 chunk_position);

	[[nodiscard]] TileTag get(Int2 position) const;

	bool set(Int2 position, TileTag tile);

	void mark_dirty() { vertices_dirty = true; }

	void draw(DrawContext& context) const;

	static Int2 get_chunk_position(Int2 position) { return { position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2 }; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	const Layer& layer;
	const Int2 chunk_position;

	static constexpr uint32_t SizeLog2 = 5;
	static constexpr uint32_t Size = 1u << SizeLog2;

private:
	void update_vertices(DrawContext& context) const;

	static uint32_t get_index(Int2 position)
	{
		Int2 local_position = get_local_position(position);

		assert(0 <= local_position.x && local_position.x < Size);
		assert(0 <= local_position.y && local_position.y < Size);
		return local_position.y * Size + local_position.x;
	}

	uint32_t occupied_tiles = 0;
	std::unique_ptr<TileType[]> tile_types;
	std::unique_ptr<uint32_t[]> tile_indices;

	mutable bool vertices_dirty = false;
	std::unique_ptr<sf::VertexBuffer> vertices_wire;
	std::unique_ptr<sf::VertexBuffer> vertices_static;
};

class DrawContext
{
public:
	DrawContext(sf::RenderWindow& window, const sf::RenderStates& states_wire, const sf::RenderStates& states_static);

	/**
	 * Emplace a new quad onto the current vertices batch.
	 */
	void emplace(bool wire, Float2 corner0, Float2 corner1, uint32_t color);

	/**
	 * Flushes batched vertices on the CPU to a buffer on GPU memory.
	 */
	void batch_buffer(bool wire, sf::VertexBuffer& buffer);

	/**
	 * Draws GPU buffer.
	 */
	void draw(bool wire, const sf::VertexBuffer& buffer);

private:
	sf::RenderWindow& window;
	const sf::RenderStates& states_wire;
	const sf::RenderStates& states_static;

	std::vector<sf::Vertex> vertices_wire;
	std::vector<sf::Vertex> vertices_static;
};

}
