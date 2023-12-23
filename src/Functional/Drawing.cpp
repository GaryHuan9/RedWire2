#include "Functional/Drawing.hpp"
#include "Utility/Functions.hpp"
#include "Utility/Types.hpp"

#include <SFML/Graphics.hpp>
#include <GL/glew.h>

namespace rw
{

DrawContext::DrawContext(sf::RenderWindow& window, const sf::RenderStates& states_wire, const sf::RenderStates& states_static) :
	window(window), states_wire(states_wire), states_static(states_static) {}

void DrawContext::emplace(bool wire, rw::Float2 corner0, rw::Float2 corner1, uint32_t color)
{
	sf::Color sf_color(color);
	auto& vertices = (wire ? vertices_wire : vertices_static);
	vertices.emplace_back(sf::Vector2f(corner0.x, corner0.y), sf_color, sf::Vector2f(0.0f, 0.0f));
	vertices.emplace_back(sf::Vector2f(corner1.x, corner0.y), sf_color, sf::Vector2f(1.0f, 0.0f));
	vertices.emplace_back(sf::Vector2f(corner1.x, corner1.y), sf_color, sf::Vector2f(1.0f, 1.0f));
	vertices.emplace_back(sf::Vector2f(corner0.x, corner1.y), sf_color, sf::Vector2f(0.0f, 1.0f));
}

void DrawContext::batch_buffer(bool wire, sf::VertexBuffer& buffer)
{
	size_t size = buffer.getVertexCount();
	auto& vertices = (wire ? vertices_wire : vertices_static);
	if (size != vertices.size()) buffer.create(vertices.size());

	buffer.update(vertices.data());
	vertices.clear();
}

void DrawContext::draw(bool wire, const sf::VertexBuffer& buffer)
{
	window.draw(buffer, wire ? states_wire : states_static);
}

DrawBuffer::DrawBuffer(void* data, size_t size, size_t count) : count(count)
{
	assert(size % count == 0);
	glGenBuffers(1, &handle);
	throw_any_gl_error();

	auto bytes = static_cast<GLsizeiptr>(size);
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
	throw_any_gl_error();
}

DrawBuffer::~DrawBuffer()
{
	assert(handle != 0);
	glDeleteBuffers(1, &handle);
	handle = 0;
}

static void set_attribute_impl(uint32_t attribute, uint32_t size, GLenum type, size_t stride, size_t offset)
{
	auto converted_size = static_cast<GLint>(size);
	auto converted_stride = static_cast<GLint>(stride);
	auto converted_offset = reinterpret_cast<void*>(offset);

	glVertexAttribPointer(attribute, converted_size, type, GL_FALSE, converted_stride, converted_offset);
	glEnableVertexAttribArray(attribute);
	throw_any_gl_error();
}

#define SET_ATTRIBUTE(Target, Size, Type)                                                \
template<>                                                                               \
void DrawBuffer::set_attribute<Target>(uint32_t attribute, size_t stride, size_t offset) \
{                                                                                        \
    set_attribute_impl(attribute, Size, Type, stride, offset);                           \
}

SET_ATTRIBUTE(float, 1, GL_FLOAT)

SET_ATTRIBUTE(Float2, 2, GL_FLOAT)

SET_ATTRIBUTE(int32_t, 1, GL_INT)

SET_ATTRIBUTE(uint32_t, 1, GL_UNSIGNED_INT)

#undef SET_ATTRIBUTE

void DrawBuffer::draw() const
{
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(count));
}

}
