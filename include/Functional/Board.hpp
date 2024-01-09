#pragma once

#include "main.hpp"
#include "Drawing.hpp"
#include "Utility/SimpleTypes.hpp"
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
	using DataTypes = TypeSet<Wire, Bridge, Gate>;
	using ListsType = DataTypes::Wrap<RecyclingList>;

	Layer();
	~Layer();

	Layer(Layer&& other) = default;

	template<class T>
	[[nodiscard]] const RecyclingList<T>& get_list() const { return lists->get<T>(); }

	template<class T>
	RecyclingList<T>& get_list() { return lists->get<T>(); }

	[[nodiscard]] Engine& get_engine() const { return *engine; }

	[[nodiscard]] TileTag get(Int2 position) const;

	[[nodiscard]] bool has(Int2 position, TileType type) const;

	void draw(DrawContext& context, Float2 min_position, Float2 max_position) const;

	void set(Int2 position, TileTag tile);

	void erase(Bounds bounds);
	Layer copy(Bounds bounds) const;

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Layer& layer);
	friend BinaryReader& operator>>(BinaryReader& reader, Layer& layer);

private:
	class Chunk;

	template<class Action>
	void for_each_chunk(Action action, Bounds bounds) const;

	static Bounds to_chunk_space(Bounds bounds); //From world / tile space to chunk space

	std::unordered_map<Int2, std::unique_ptr<Chunk>> chunks;
	std::unique_ptr<ListsType> lists;
	std::unique_ptr<Engine> engine;
};

class Layer::Chunk
{
public:
	explicit Chunk(Int2 chunk_position);

	Chunk(const Chunk& other);

	[[nodiscard]] TileTag get(Int2 tile_index) const;

	[[nodiscard]] uint32_t count() const;

	void draw(DrawContext& context) const;

	bool set(Int2 position, TileTag tile);

	void update_draw_buffer(DrawContext& context, const Layer& layer);

	void write(BinaryWriter& writer) const;
	void read(BinaryReader& reader);

	static Int2 get_chunk_position(Int2 position) { return { position.x >> Chunk::SizeLog2, position.y >> Chunk::SizeLog2 }; }

	static Int2 get_local_position(Int2 position) { return { position.x & (Chunk::Size - 1), position.y & (Chunk::Size - 1) }; }

	const Int2 chunk_position; //Note that this is the position in chunk space (i.e. not multiplied with size)

	static constexpr uint32_t SizeLog2 = 5;
	static constexpr uint32_t Size = 1u << SizeLog2;
	static constexpr uint32_t Size2 = Size * Size;

private:
	[[nodiscard]] TileTag get(size_t tile_index) const;

	static size_t get_tile_index(Int2 position)
	{
		Int2 local_position = get_local_position(position);

		assert(0 <= local_position.x && local_position.x < Size);
		assert(0 <= local_position.y && local_position.y < Size);
		return local_position.y * Size + local_position.x;
	}

	uint32_t occupied_tiles = 0;
	std::unique_ptr<std::array<TileType, Size2>> tile_types;
	std::unique_ptr<std::array<uint32_t, Size2>> tile_indices;

	bool vertices_dirty = false;
	VertexBuffer vertex_buffer_quad;
	VertexBuffer vertex_buffer_wire;
};

}
