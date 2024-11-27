#pragma once

#include "RenderLayer.hpp"
#include "Utility/BasicTypes.hpp"

#include <unordered_map>
#include <vector>

#include <bgfx/bgfx.h>

class GLFWwindow;

namespace rw
{
class RenderWindow
{
public:
    RenderWindow(Int2 size, const std::string& name);
    ~RenderWindow();

    GLFWwindow* get_handle() const { return handle; }

    bgfx::ProgramHandle get_shader(const std::string& name) const;
    bgfx::ViewId get_view_id(RenderLayer layer) const;

    void update();
    void render();

private:
    void initialize();

    GLFWwindow* handle;
    Int2 window_size;

    std::unordered_map<std::string, bgfx::ProgramHandle> shaders;
    std::vector<bgfx::ViewId> view_ids; //Aligns to the View enum
};
} // rw
