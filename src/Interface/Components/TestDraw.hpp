#pragma once
#include "Interface/Application/Component.hpp"

#include <bgfx/bgfx.h>

namespace rw
{
class TestDraw final : public Component
{
public:
    explicit TestDraw(Application& application);
    ~TestDraw() override;

    void initialize() override;
    void update() override;
    void render() override;

private:
    bgfx::VertexBufferHandle vertex_buffer{};
    bgfx::IndexBufferHandle index_buffer{};
};
} // rw
