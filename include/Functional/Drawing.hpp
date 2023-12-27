#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"

#include <array>
#include <tuple>
#include <unordered_map>

using GLuint = unsigned int;
using GLenum = unsigned int;

namespace rw
{

class DrawContext
{
public:
	DrawContext(sf::Shader* shader_quad, sf::Shader* shader_wire) : shader_quad(shader_quad), shader_wire(shader_wire) {}

	void emplace_quad(Float2 corner0, Float2 corner1, uint32_t color);
	void emplace_wire(Float2 corner0, Float2 corner1, uint32_t color);

	[[nodiscard]] VertexBuffer flush_buffer(bool quad);

	void draw(bool quad, const VertexBuffer& buffer) const;

	void update_wire_states() const;

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
		uint32_t color{};
	};

	std::vector<QuadVertex> vertices_quad;
	std::vector<WireVertex> vertices_wire;

	sf::Shader* shader_quad;
	sf::Shader* shader_wire;
};

class DataBuffer
{
public:
	DataBuffer();

	DataBuffer(GLenum type, GLenum usage);

	DataBuffer(DataBuffer&& value) noexcept : DataBuffer() { swap(*this, value); }

	DataBuffer& operator=(DataBuffer&& value) noexcept
	{
		swap(*this, value);
		return *this;
	}

	~DataBuffer();

	DataBuffer(const DataBuffer&) = delete;
	DataBuffer& operator=(const DataBuffer&) = delete;

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

	friend void swap(DataBuffer& value, DataBuffer& other) noexcept;

private:
	[[nodiscard]]
	bool empty() const
	{
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
	size_t size;
};

class VertexBuffer
{
public:
	VertexBuffer();

	VertexBuffer(VertexBuffer&& value) noexcept : VertexBuffer() { swap(*this, value); }

	VertexBuffer& operator=(VertexBuffer&& value) noexcept
	{
		swap(*this, value);
		return *this;
	}

	~VertexBuffer();

	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;

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
	size_t count;
	DataBuffer data;
};

} // rw
