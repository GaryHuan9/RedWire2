#include "Functional/Drawing.hpp"
#include "Functional/Tiles.hpp"
#include "Utility/Functions.hpp"
#include "Utility/SimpleTypes.hpp"

#include "SFML/Graphics.hpp"
#include "GL/glew.h"

#include <fstream>

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

ShaderResources::ShaderResources() :
	quads(std::make_unique<decltype(quads)::element_type>()),
	wires(std::make_unique<decltype(wires)::element_type>())
{
	auto read_string = [](const std::string& path)
	{
		std::ifstream stream(path);
		std::stringstream buffer;

		if (not stream.good()) throw std::runtime_error("Unable to open stream.");
		buffer << stream.rdbuf();
		return buffer.str();
	};

	auto find_region = [](std::string_view view, std::string_view label) -> std::string_view
	{
		auto next_symbol = [](std::string_view& view) -> std::string_view
		{
			auto iterator = std::find_if_not(view.begin(), view.end(), std::iswspace);
			if (iterator == view.end() || iterator == view.begin()) return {};
			view = view.substr(std::distance(view.begin(), iterator));

			iterator = std::find_if(view.begin(), view.end(), std::iswspace);
			size_t index = std::distance(view.begin(), iterator);

			auto result = view.substr(0, index);
			view = view.substr(index);
			return result;
		};

		static constexpr std::string_view Keyword = "#define";

		size_t index = view.find(Keyword);

		while (index < view.size())
		{
			view = view.substr(index + Keyword.size());
			bool found = next_symbol(view) == label;
			index = view.find(Keyword);

			if (found) return view.substr(0, index);
		}

		return {};
	};

	auto compile = [](sf::Shader& shader, const std::string& vertex, const std::string& fragment)
	{
		if (shader.loadFromMemory(vertex, fragment)) return;
		throw std::runtime_error("Failed to compile shaders.");
	};

	std::string vertex_quad = read_string("rsc/Tiles/Quad.vert");
	std::string vertex_wire = read_string("rsc/Tiles/Wire.vert");
	std::string fragment = read_string("rsc/Tiles/Tile.frag");
	std::string position = read_string("rsc/Tiles/Position.glsl");

	TileRotation rotation;

	do
	{
		std::string_view view = find_region(position, "R" + std::to_string(rotation.get_value()));
		if (view.empty()) throw std::runtime_error("Unable to find cross compile region for shader.");

		compile((*quads)[rotation.get_value()], std::string(vertex_quad).append(view), fragment);
		compile((*wires)[rotation.get_value()], std::string(vertex_wire).append(view), fragment);
		rotation = rotation.get_next();
	}
	while (rotation != TileRotation());
}

sf::Shader* ShaderResources::get_shader(bool quad, TileRotation rotation) const
{
	return &(quad ? quads : wires)->at(rotation.get_value());
}

DrawContext::DrawContext(const ShaderResources& shaders) :
	shaders(shaders), wire_states_buffer(GL_SHADER_STORAGE_BUFFER, GL_STREAM_DRAW)
{
	set_rotation(TileRotation());
}

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

void DrawContext::set_rotation(TileRotation new_rotation)
{
	rotation = new_rotation;
	shader_quad = shaders.get_shader(true, rotation);
	shader_wire = shaders.get_shader(false, rotation);
	shader_dirty = true;
}

void DrawContext::set_view(Float2 center, Float2 extend)
{
	scale = Float2(1.0f) / extend;
	origin = -center * scale;
	shader_dirty = true;
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
		position = rotation.rotate(position);
		position = position * scale + origin; //Note that scale and origin transforms to clip space
		position = position * 0.5f + Float2(0.5f);
		position = position.max(Float2(0.0f)).min(Float2(1.0f));
		return position;
	};

	Float2 corner0 = transform(min_position);
	Float2 corner1 = transform(max_position);
	Float2 min = corner0.min(corner1);
	Float2 max = corner0.max(corner1);

	//Get screen size
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	throw_any_gl_error();
	Float2 screen(viewport[2], viewport[3]);

	//Transform from clip space to screen space
	Int2 min_pixel = Float2::floor(min * screen);
	Int2 max_pixel = Float2::floor(max * screen) + Int2(1);
	Int2 size = max_pixel - min_pixel;

	//Apply clip
	glScissor(min_pixel.x, min_pixel.y, size.x, size.y);
	glEnable(GL_SCISSOR_TEST);
	throw_any_gl_error();
}

void DrawContext::draw(bool quad, const VertexBuffer& buffer) const
{
	set_shader_parameters();

	if (quad)
	{
		sf::Shader::bind(shader_quad);
		buffer.draw();
	}
	else
	{
		sf::Shader::bind(shader_wire);
		wire_states_buffer.bind_base(0);
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

void DrawContext::set_shader_parameters() const
{
	if (not shader_dirty) return;
	shader_dirty = false;

	shader_quad->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_quad->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));

	shader_wire->setUniform("scale", sf::Glsl::Vec2(scale.x, scale.y));
	shader_wire->setUniform("origin", sf::Glsl::Vec2(origin.x, origin.y));
}

}
