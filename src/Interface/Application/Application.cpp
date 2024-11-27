#include "Application.hpp"
#include "Component.hpp"
#include "ImGuiBackend.hpp"

#include "Graphics/RenderWindow.hpp"
// #include "Interface/Components/GridView.hpp"
#include "Interface/Components/TestDraw.hpp"
#include "Utility/BasicTypes.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

namespace rw
{
Application::Application() : render_window(std::make_unique<RenderWindow>(Int2(1920, 1080), "RedWire2")),
                             imgui_backend(std::make_unique<ImGuiBackend>(*render_window))
{
    // components.emplace_back(std::make_unique<GridView>(*this));
    components.emplace_back(std::make_unique<TestDraw>(*this));

    for (const auto& component : components) component->initialize();
}

Application::~Application() = default;

bool Application::alive() const { return not glfwWindowShouldClose(render_window->get_handle()); }

void Application::update()
{
    glfwPollEvents();
    render_window->update();
    imgui_backend->update();

    for (const auto& component : components) component->update();
    ImGui::ShowDemoWindow();

    for (const auto& component : components) component->render();

    imgui_backend->render();
    render_window->render();
}
} // rw
