#pragma once

#include "main.hpp"

#include <array>
#include <tuple>
#include <unordered_map>

namespace rw
{

using GLuint = unsigned int;

class DrawContext
{
public:
	DrawContext(sf::RenderWindow& window, const sf::RenderStates& states_wire, const sf::RenderStates& states_static);

	/**
	 * Emplace a new quad onto the current vertices batch.
	 */
	void emplace(bool wire, rw::Float2 corner0, rw::Float2 corner1, uint32_t color);

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

class DrawBuffer
{
public:
	template<class T>
	DrawBuffer(const T* data, size_t count) : DrawBuffer(static_cast<void*>(data), count * sizeof(T), count) {}

	~DrawBuffer();

	DrawBuffer(const DrawBuffer&) = delete;
	DrawBuffer& operator=(const DrawBuffer&) = delete;

	template<class T>
	void set_attribute(uint32_t attribute, size_t stride, size_t offset);

	template<class... Ts>
	void set_attributes()
	{
		size_t stride = get_total_sizeof<Ts...>();
		set_attributes_impl<Ts...>(0, stride, 0);
	}

	void draw() const;

private:
	DrawBuffer(void* data, size_t size, size_t count);

	template<class T, class... Ts>
	void set_attributes_impl(uint32_t attribute, size_t stride, size_t offset)
	{
		set_attribute<T>(attribute, stride, offset);
		if constexpr (sizeof...(Ts) == 0) return;
		else set_attributes_impl<Ts...>(attribute, stride, offset + sizeof(T));
	}

	template<class T, class... Ts>
	size_t get_total_sizeof()
	{
		if constexpr (sizeof...(Ts) == 0) return sizeof(T);
		else return sizeof(T) + get_total_sizeof<Ts...>();
	}

	GLuint handle{};
	size_t count;
};

} // rw
