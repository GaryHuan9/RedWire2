#include "Functional/Drawing.hpp"
#include "Utility/Functions.hpp"
#include "Utility/SimpleTypes.hpp"

#include "SFML/Graphics.hpp"
#include "GL/glew.h"

namespace rw
{

DataBuffer::DataBuffer(GLenum type, GLenum usage) : type(type), usage(usage), handle(0), size(0) { assert(empty()); }

DataBuffer::DataBuffer() : type(GLenum()), usage(GLenum()), handle(0), size(0) {}

DataBuffer::~DataBuffer()
{
	if (not valid() || empty()) return;
	glDeleteBuffers(1, &handle);
	throw_any_gl_error();
	handle = 0;
}

void DataBuffer::update_impl(const void* data, size_t new_size)
{
	assert(valid());

	uint32_t old_size = size;
	size = static_cast<uint32_t>(new_size);
	assert(size == new_size);

	auto casted_size = static_cast<GLsizeiptr>(size);

	if (size == 0)
	{
		if (old_size == 0)
		{
			assert(handle == 0);
			return;
		}

		assert(handle != 0);
		glDeleteBuffers(1, &handle);
		throw_any_gl_error();

		handle = 0;
		return;
	}

	if (size == old_size)
	{
		assert(old_size != 0);
		assert(handle != 0);

		bind();
		glBufferSubData(type, 0, casted_size, data);
		throw_any_gl_error();
		return;
	}

	if (old_size == 0)
	{
		assert(handle == 0);
		glGenBuffers(1, &handle);
		throw_any_gl_error();
	}

	bind();
	glBufferData(type, casted_size, data, usage);
	throw_any_gl_error();
}

#define SET_ATTRIBUTE(Target, Size, Type, Integer)                                             \
template<>                                                                                     \
void DataBuffer::set_attribute<Target>(uint32_t attribute, size_t stride, size_t offset) const \
{                                                                                              \
    set_attribute_impl(attribute, Size, Type, Integer, stride, offset);                        \
}

SET_ATTRIBUTE(float, 1, GL_FLOAT, false)

SET_ATTRIBUTE(Float2, 2, GL_FLOAT, false)

SET_ATTRIBUTE(int32_t, 1, GL_INT, true)

SET_ATTRIBUTE(uint32_t, 1, GL_UNSIGNED_INT, true)

#undef SET_ATTRIBUTE

void DataBuffer::bind() const
{
	if (empty()) return;
	glBindBuffer(type, handle);
	throw_any_gl_error();
}

void DataBuffer::bind_base(size_t index) const
{
	if (empty()) return;
	glBindBufferBase(type, index, handle);
	throw_any_gl_error();
}

void DataBuffer::unbind() const
{
	if (empty()) return;
	glBindBuffer(type, 0);
	throw_any_gl_error();
}

void swap(DataBuffer& value, DataBuffer& other) noexcept
{
	using std::swap;

	swap(value.type, other.type);
	swap(value.usage, other.usage);
	swap(value.handle, other.handle);
	swap(value.size, other.size);
}

void DataBuffer::set_attribute_impl(uint32_t attribute, uint32_t attribute_size, GLenum attribute_type,
                                    bool integer, size_t stride, size_t offset) const
{
	if (empty()) return;
	bind();

	auto casted_size = static_cast<GLint>(attribute_size);
	auto casted_stride = static_cast<GLint>(stride);
	auto casted_offset = reinterpret_cast<void*>(offset);

	//@formatter:off
	if (integer) glVertexAttribIPointer(attribute, casted_size, attribute_type, casted_stride, casted_offset);
	else glVertexAttribPointer(attribute, casted_size, attribute_type, GL_FALSE, casted_stride, casted_offset);
	//@formatter:on

	glEnableVertexAttribArray(attribute);
	throw_any_gl_error();
}

VertexBuffer::VertexBuffer() : handle(0), count(0), data(GL_ARRAY_BUFFER, GL_STATIC_DRAW) {}

VertexBuffer::~VertexBuffer()
{
	if (empty()) return;
	glDeleteVertexArrays(1, &handle);
	throw_any_gl_error();
	handle = 0;
}

void VertexBuffer::bind() const
{
	if (empty()) return;
	glBindVertexArray(handle);
	throw_any_gl_error();
	data.bind();
}

void VertexBuffer::draw() const
{
	if (empty()) return;
	assert(count % 4 == 0);

	bind();
	glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(count));
	throw_any_gl_error();
}

void swap(VertexBuffer& value, VertexBuffer& other) noexcept
{
	using std::swap;

	swap(value.handle, other.handle);
	swap(value.count, other.count);
	swap(value.data, other.data);
}

void VertexBuffer::update_impl(size_t new_count)
{
	uint32_t old_count = count;
	count = static_cast<uint32_t>(new_count);
	assert(count == new_count);

	if (old_count == 0)
	{
		assert(handle == 0);
		if (count == 0) return;
		glGenVertexArrays(1, &handle);
		throw_any_gl_error();
	}

	if (count == 0)
	{
		assert(handle != 0);
		assert(old_count != 0);

		glDeleteVertexArrays(1, &handle);
		throw_any_gl_error();
		handle = 0;
		return;
	}

	assert(handle != 0);
	bind();
}

DrawContext::DrawContext(std::unique_ptr<sf::Shader>&& shader_quad, std::unique_ptr<sf::Shader>&& shader_wire) :
	shader_quad(std::move(shader_quad)), shader_wire(std::move(shader_wire)),
	wire_states_buffer(GL_SHADER_STORAGE_BUFFER, GL_STREAM_DRAW) {}

void DrawContext::emplace_quad(Float2 corner0, Float2 corner1, uint32_t color)
{
	uint32_t value = swap_endianness(color);
	vertices_quad.emplace_back(Float2(corner0.x, corner0.y), value);
	vertices_quad.emplace_back(Float2(corner1.x, corner0.y), value);
	vertices_quad.emplace_back(Float2(corner1.x, corner1.y), value);
	vertices_quad.emplace_back(Float2(corner0.x, corner1.y), value);
}

void DrawContext::emplace_wire(Float2 corner0, Float2 corner1, Index wire_index)
{
	uint32_t value = wire_index;
	vertices_wire.emplace_back(Float2(corner0.x, corner0.y), value);
	vertices_wire.emplace_back(Float2(corner1.x, corner0.y), value);
	vertices_wire.emplace_back(Float2(corner1.x, corner1.y), value);
	vertices_wire.emplace_back(Float2(corner0.x, corner1.y), value);
}

VertexBuffer DrawContext::flush_buffer(bool quad)
{
	auto impl = []<class T>(std::vector<T>& vertices)
	{
		VertexBuffer buffer;
		buffer.update(vertices.data(), vertices.size());
		buffer.set_attributes<Float2, uint32_t>();

		vertices.clear();
		return std::move(buffer);
	};

	return quad ? impl(vertices_quad) : impl(vertices_wire);
}

void DrawContext::set_view(Float2 new_scale, Float2 new_origin)
{
	scale = new_scale;
	origin = new_origin;

	shader_quad->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_quad->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));

	shader_wire->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_wire->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));
}

void DrawContext::set_wire_states(const void* data, size_t size)
{
	auto casted = reinterpret_cast<const uint8_t*>(data);
	wire_states_buffer.update<uint8_t>(casted, size);
	wire_states_buffer.unbind();
}

void DrawContext::clip(Float2 min_position, Float2 max_position) const
{
	//Transform position from world space to clip space
	auto transform = [this](Float2 position)
	{
		//Note that scale and origin are in clip space
		position = position * scale + origin;
		position = position * Float2(0.5f, -0.5f) + Float2(0.5f);
		position = position.max(Float2(0.0f)).min(Float2(1.0f));
		return position;
	};

	min_position = transform(min_position);
	max_position = transform(max_position);

	//Get screen size
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	throw_any_gl_error();
	Float2 screen(viewport[2], viewport[3]);

	//Transform from clip space to screen space
	Int2 min_pixel = Float2::floor(min_position * screen);
	Int2 max_pixel = Float2::floor(max_position * screen) + Int2(1);
	Int2 size = max_pixel - min_pixel;

	//Apply clip
	glScissor(min_pixel.x, min_pixel.y, size.x, size.y);
	glEnable(GL_SCISSOR_TEST);
	throw_any_gl_error();
}

void DrawContext::draw(bool quad, const VertexBuffer& buffer) const
{
	if (quad)
	{
		sf::Shader::bind(shader_quad.get());
		buffer.draw();
	}
	else
	{
		sf::Shader::bind(shader_wire.get());
		wire_states_buffer.bind_base(2);
		buffer.draw();
		wire_states_buffer.unbind();
	}
}

void DrawContext::clear()
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_SCISSOR_TEST);
	sf::Shader::bind(nullptr);
	throw_any_gl_error();

	vertices_quad.clear();
	vertices_wire.clear();
}

}
