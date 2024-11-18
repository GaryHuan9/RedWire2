#pragma once

#include <bgfx/bgfx.h>

class GLFWwindow;
class GLFWcursor;
class ImDrawData;

namespace rw
{

class RenderWindow;

class ImGuiBackend
{
public:
    explicit ImGuiBackend(const RenderWindow& window);
    ~ImGuiBackend();

    void update();
    void render();

private:
    const RenderWindow& window;
    GLFWwindow* handle;

    bgfx::VertexLayout vertex_layout{};
    bgfx::TextureHandle font_texture{};
    bgfx::UniformHandle font_uniform{};
};
} // rw
