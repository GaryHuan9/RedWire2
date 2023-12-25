#include "Functional/Drawing.hpp"
#include "Utility/Functions.hpp"
#include "Utility/Types.hpp"

#include <SFML/Graphics.hpp>
#include <GL/glew.h>

namespace rw
{

void DrawContext::emplace_quad(Float2 corner0, Float2 corner1, uint32_t color)
{
	color = swap_endianness(color);
	vertices_quad.emplace_back(Float2(corner0.x, corner0.y), color);
	vertices_quad.emplace_back(Float2(corner1.x, corner0.y), color);
	vertices_quad.emplace_back(Float2(corner1.x, corner1.y), color);
	vertices_quad.emplace_back(Float2(corner0.x, corner1.y), color);
}

void DrawContext::emplace_wire(Float2 corner0, Float2 corner1, uint32_t color)
{
	color = swap_endianness(color);
	vertices_wire.emplace_back(Float2(corner0.x, corner0.y), color);
	vertices_wire.emplace_back(Float2(corner1.x, corner0.y), color);
	vertices_wire.emplace_back(Float2(corner1.x, corner1.y), color);
	vertices_wire.emplace_back(Float2(corner0.x, corner1.y), color);
}

DrawBuffer DrawContext::flush_buffer(bool quad)
{
	auto impl = []<class T>(std::vector<T>& vertices)
	{
		DrawBuffer buffer(vertices.data(), vertices.size());
		buffer.set_attributes<Float2, uint32_t>();

		vertices.clear();
		return std::move(buffer);
	};

	return quad ? impl(vertices_quad) : impl(vertices_wire);
}

void DrawContext::draw(bool quad, const DrawBuffer& buffer) const
{
	sf::Shader::bind(quad ? shader_quad : shader_wire);
	buffer.draw();
}

void DrawContext::clear()
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	sf::Shader::bind(nullptr);

	vertices_quad.clear();
	vertices_wire.clear();
}

DrawBuffer::DrawBuffer(const void* data, size_t size, size_t count) : count(count)
{
	if (count == 0) return;
	assert(size % count == 0);

	glGenVertexArrays(1, &handle_vao);
	glBindVertexArray(handle_vao);
	throw_any_gl_error();

	glGenBuffers(1, &handle_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, handle_vbo);
	throw_any_gl_error();

	auto bytes = static_cast<GLsizeiptr>(size);
	glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
	throw_any_gl_error();
}

DrawBuffer::~DrawBuffer()
{
	if (count == 0) return;
	assert(handle_vao != 0);
	assert(handle_vbo != 0);

	glDeleteVertexArrays(1, &handle_vao);
	glDeleteBuffers(1, &handle_vbo);
	throw_any_gl_error();

	handle_vao = {};
	handle_vbo = {};
}

#define SET_ATTRIBUTE(Target, Size, Type, Integer)                                             \
template<>                                                                                     \
void DrawBuffer::set_attribute<Target>(uint32_t attribute, size_t stride, size_t offset) const \
{                                                                                              \
    set_attribute_impl(attribute, Size, Type, Integer, stride, offset);                        \
}

SET_ATTRIBUTE(float, 1, GL_FLOAT, false)

SET_ATTRIBUTE(Float2, 2, GL_FLOAT, false)

SET_ATTRIBUTE(int32_t, 1, GL_INT, true)

SET_ATTRIBUTE(uint32_t, 1, GL_UNSIGNED_INT, true)

#undef SET_ATTRIBUTE

void DrawBuffer::draw() const
{
	if (count == 0) return;
	assert(count % 4 == 0);
	assert(handle_vao != 0);
	assert(handle_vbo != 0);

	glBindVertexArray(handle_vao);
	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(count));
	throw_any_gl_error();
}

void swap(DrawBuffer& value, DrawBuffer& other) noexcept
{
	using std::swap;

	swap(value.count, other.count);
	swap(value.handle_vao, other.handle_vao);
	swap(value.handle_vbo, other.handle_vbo);
}

void DrawBuffer::set_attribute_impl(uint32_t attribute, uint32_t size, GLenum type, bool integer, size_t stride, size_t offset) const
{
	if (count == 0) return;

	glBindVertexArray(handle_vao);
	glBindBuffer(GL_ARRAY_BUFFER, handle_vbo);
	throw_any_gl_error();

	auto converted_size = static_cast<GLint>(size);
	auto converted_stride = static_cast<GLint>(stride);
	auto converted_offset = reinterpret_cast<void*>(offset);

	//@formatter:off
	if (integer) glVertexAttribIPointer(attribute, converted_size, type, converted_stride, converted_offset);
	else glVertexAttribPointer(attribute, converted_size, type, GL_FALSE, converted_stride, converted_offset);
	//@formatter:on

	glEnableVertexAttribArray(attribute);
	throw_any_gl_error();
}

}
