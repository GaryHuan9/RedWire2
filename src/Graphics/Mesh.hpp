#pragma once

#include "RenderMemory.hpp"

#include <bgfx/bgfx.h>

namespace rw
{
class Mesh
{
public:
    Mesh(bool dynamic, bgfx::ProgramHandle shader, bgfx::ViewId view_id);
    ~Mesh();

    void render();

private:
    bool dynamic;
    bgfx::ProgramHandle shader;
    bgfx::ViewId view_id;

    bgfx::DynamicVertexBufferHandle dynamic_vertices{};
    bgfx::DynamicIndexBufferHandle dynamic_indices{};

    bgfx::VertexBufferHandle static_vertices{};
    bgfx::IndexBufferHandle static_indices{};
};
} // rw
