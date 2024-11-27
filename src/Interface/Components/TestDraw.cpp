#include "TestDraw.hpp"

#include "Graphics/RenderWindow.hpp"
#include "Interface/Application/Application.hpp"

#include <bgfx/bgfx.h>

namespace rw
{
struct Vertex
{
    Float2 position;
    uint32_t color;
};

std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, 0xFF339933},
    {{0.5f, -0.5f}, 0xFF993333},
    {{0.0f, 0.5f}, 0xFF333399},
};

std::vector<uint16_t> indices = {
    0, 1, 2
};

TestDraw::TestDraw(Application& application) : Component(application)
{
    bgfx::VertexLayout vertex_layout;
    vertex_layout
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

    vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(vertices.data(), vertices.size() * sizeof(Vertex)),
                                             vertex_layout);
    index_buffer = bgfx::createIndexBuffer(bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t)));
}

TestDraw::~TestDraw() = default;

void TestDraw::initialize() {}

void TestDraw::update() {}

void TestDraw::render()
{
    RenderWindow& window = application.get_render_window();
    auto shader = window.get_shader("Default");
    auto view_id = window.get_view_id(RenderLayer::Components);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);

    bgfx::setVertexBuffer(0, vertex_buffer);
    bgfx::setIndexBuffer(index_buffer);
    bgfx::submit(view_id, shader);
}
} // rw
