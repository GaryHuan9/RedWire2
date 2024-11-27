#include "Mesh.hpp"

namespace rw
{
Mesh::Mesh(bool dynamic, bgfx::ProgramHandle shader, bgfx::ViewId view_id)
    : dynamic(dynamic), shader(shader), view_id(view_id) {}

Mesh::~Mesh()
{
    if (dynamic)
    {
        bgfx::destroy(dynamic_vertices);
        bgfx::destroy(dynamic_indices);
    }
    else
    {
        bgfx::destroy(static_vertices);
        bgfx::destroy(static_indices);
    }
}

void Mesh::render()
{
    if (dynamic)
    {
        bgfx::setVertexBuffer(0, dynamic_vertices);
        bgfx::setIndexBuffer(dynamic_indices);
    }
    else
    {
        bgfx::setVertexBuffer(0, static_vertices);
        bgfx::setIndexBuffer(static_indices);
    }
    bgfx::submit(view_id, shader);
}
} // rw
