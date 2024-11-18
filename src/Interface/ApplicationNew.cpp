#include "Interface/ApplicationNew.hpp"

#include "Interface/ImGuiBackend.hpp"
#include "Interface/RenderWindow.hpp"
#include "Utility/SimpleTypes.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

namespace rw
{
ApplicationNew::ApplicationNew() : render_window(std::make_unique<RenderWindow>(Int2(1920, 1080), "RedWire2")),
                                   imgui_backend(std::make_unique<ImGuiBackend>(*render_window)) {}

ApplicationNew::~ApplicationNew() = default;

bool ApplicationNew::alive() const { return not glfwWindowShouldClose(render_window->get_handle()); }

void ApplicationNew::update()
{
    glfwPollEvents();
    render_window->update();
    imgui_backend->update();

    ImGui::ShowDemoWindow();

    imgui_backend->render();
    render_window->render();
}
} // rw
