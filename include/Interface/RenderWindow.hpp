#pragma once

#include "main.hpp"
#include "Utility/SimpleTypes.hpp"

#include <unordered_map>
#include <vector>

class GLFWwindow;

namespace bgfx
{
class ProgramHandle;
typedef uint16_t ViewId;
}

namespace rw
{
class RenderWindow
{
public:
    enum class View;

    RenderWindow(Int2 size, const std::string& name);
    ~RenderWindow();

    GLFWwindow* get_handle() const { return handle; }

    bgfx::ProgramHandle get_shader(const std::string& name) const;
    bgfx::ViewId get_view_id(View view) const;

    void update();
    void render();

    enum class View
    {
        Background = 0,
        Components = 1,
        Interface = 2
    };

private:
    void initialize();

    GLFWwindow* handle;
    Int2 window_size;

    std::unordered_map<std::string, bgfx::ProgramHandle> shaders;
    std::vector<bgfx::ViewId> view_ids; //Aligns to the View enum
};
} // rw
