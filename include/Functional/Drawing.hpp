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

	[[nodiscard]] DrawBuffer flush_buffer(bool quad);

	void draw(bool quad, const DrawBuffer& buffer) const;

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

class DrawBuffer
{
public:
	DrawBuffer() : DrawBuffer(nullptr, 0, 0) {}

	template<class T>
	DrawBuffer(const T* data, size_t count) : DrawBuffer(reinterpret_cast<const void*>(data), count * sizeof(T), count) {}

	DrawBuffer(DrawBuffer&& value) noexcept : DrawBuffer() { swap(*this, value); }

	DrawBuffer& operator=(DrawBuffer&& value) noexcept
	{
		swap(*this, value);
		return *this;
	}

	~DrawBuffer();

	DrawBuffer(const DrawBuffer&) = delete;
	DrawBuffer& operator=(const DrawBuffer&) = delete;

	template<class T>
	void set_attribute(uint32_t attribute, size_t stride, size_t offset) const;

	template<class... Ts>
	void set_attributes() const
	{
		size_t stride = get_total_sizeof<Ts...>();
		set_attributes_impl<Ts...>(0, stride, 0);
	}

	void draw() const;

	friend void swap(DrawBuffer& value, DrawBuffer& other) noexcept;

private:
	DrawBuffer(const void* data, size_t size, size_t count);

	void set_attribute_impl(uint32_t attribute, uint32_t size, GLenum type, bool integer, size_t stride, size_t offset) const;

	template<class T, class... Ts>
	void set_attributes_impl(uint32_t attribute, size_t stride, size_t offset) const
	{
		set_attribute<T>(attribute, stride, offset);
		if constexpr (sizeof...(Ts) == 0) return;
		else set_attributes_impl<Ts...>(attribute + 1, stride, offset + sizeof(T));
	}

	template<class T, class... Ts>
	static size_t get_total_sizeof()
	{
		if constexpr (sizeof...(Ts) == 0) return sizeof(T);
		else return sizeof(T) + get_total_sizeof < Ts...>();
	}

	size_t count;
	GLuint handle_vao{};
	GLuint handle_vbo{};
};

} // rw
