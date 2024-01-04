#pragma once

#include "main.hpp"
#include "Utility/SimpleTypes.hpp"

#include <array>
#include <tuple>
#include <unordered_map>

using GLuint = unsigned int;
using GLenum = unsigned int;

namespace rw
{

class DataBuffer : NonCopyable
{
public:
	DataBuffer(DataBuffer&& other) noexcept : DataBuffer() { swap(*this, other); }

	DataBuffer(GLenum type, GLenum usage);

	DataBuffer();
	~DataBuffer();

	DataBuffer& operator=(DataBuffer&& other) noexcept
	{
		swap(*this, other);
		return *this;
	}

	template<class T>
	void update(const T* data, size_t count)
	{
		update_impl(reinterpret_cast<const void*>(data), count * sizeof(T));
	}

	template<class T>
	void set_attribute(uint32_t attribute, size_t stride, size_t offset) const;

	template<class... Ts>
	void set_attributes() const
	{
		size_t stride = get_total_sizeof<Ts...>();
		set_attributes_impl<Ts...>(0, stride, 0);
	}

	void bind() const;

	void bind_base(size_t index) const;

	void unbind() const;

	friend void swap(DataBuffer& value, DataBuffer& other) noexcept;

private:
	[[nodiscard]] bool valid() const { return type != GLenum() && usage != GLenum(); }

	[[nodiscard]]
	bool empty() const
	{
		assert(valid());
		bool result = handle == 0;
		assert((size == 0) == result);
		return result;
	}

	void update_impl(const void* data, size_t new_size);

	void set_attribute_impl(uint32_t attribute, uint32_t attribute_size, GLenum attribute_type,
	                        bool integer, size_t stride, size_t offset) const;

	template<class T, class... Ts>
	void set_attributes_impl(uint32_t attribute, size_t stride, size_t offset) const
	{
		set_attribute<T>(attribute, stride, offset);
		if constexpr (sizeof...(Ts) == 0) return;
		else set_attributes_impl < Ts...>(attribute + 1, stride, offset + sizeof(T));
	}

	template<class T, class... Ts>
	static size_t get_total_sizeof()
	{
		if constexpr (sizeof...(Ts) == 0) return sizeof(T);
		else return sizeof(T) + get_total_sizeof < Ts...>();
	}

	GLenum type;
	GLenum usage;
	GLuint handle;
	uint32_t size;
};

class VertexBuffer : NonCopyable
{
public:
	VertexBuffer();
	~VertexBuffer();

	VertexBuffer(VertexBuffer&& other) noexcept : VertexBuffer() { swap(*this, other); }

	VertexBuffer& operator=(VertexBuffer&& other) noexcept
	{
		swap(*this, other);
		return *this;
	}

	template<class T>
	void update(const T* new_data, size_t new_count)
	{
		update_impl(new_count);
		data.update(new_data, new_count);
	}

	template<class T>
	void set_attribute(uint32_t attribute, size_t stride, size_t offset) const
	{
		bind();
		data.set_attribute<T>(attribute, stride, offset);
	}

	template<class... Ts>
	void set_attributes() const
	{
		bind();
		data.set_attributes<Ts...>();
	}

	void bind() const;

	void draw() const;

	friend void swap(VertexBuffer& value, VertexBuffer& other) noexcept;

private:
	[[nodiscard]]
	bool empty() const
	{
		bool result = handle == 0;
		assert((count == 0) == result);
		return result;
	}

	void update_impl(size_t new_count);

	GLuint handle;
	uint32_t count;
	DataBuffer data;
};

class DrawContext
{
public:
	DrawContext(sf::Shader* shader_quad, sf::Shader* shader_wire);

	void emplace_quad(Float2 corner0, Float2 corner1, uint32_t color);
	void emplace_wire(Float2 corner0, Float2 corner1, Index wire_index);

	[[nodiscard]] VertexBuffer flush_buffer(bool quad);

	void draw(bool quad, const VertexBuffer& buffer) const;

	void update_wire_states(const void* data, size_t size);

	void clear();

public:
	struct QuadVertex
	{
		Float2 position;
		uint32_t color{};
	};

	struct WireVertex
	{
		Float2 position;
		uint32_t index{};
	};

	std::vector<QuadVertex> vertices_quad;
	std::vector<WireVertex> vertices_wire;

	sf::Shader* shader_quad;
	sf::Shader* shader_wire;
	DataBuffer wire_states_buffer;
};

} // rw
