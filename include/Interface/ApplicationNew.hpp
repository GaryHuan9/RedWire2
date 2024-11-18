#pragma once
#include "main.hpp"

namespace rw
{
class RenderWindow;
class ImGuiBackend;

class ApplicationNew
{
public:
    ApplicationNew();
    ~ApplicationNew();

    bool alive() const;
    void update();

private:
    std::unique_ptr<RenderWindow> render_window;
    std::unique_ptr<ImGuiBackend> imgui_backend;
};
} // rw
