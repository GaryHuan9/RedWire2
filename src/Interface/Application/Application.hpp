#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace rw
{
class RenderWindow;
class ImGuiBackend;
class Component;

class Application
{
public:
    Application();
    ~Application();

    RenderWindow& get_render_window() const { return *render_window; }

    bool alive() const;
    void update();

	template<class T>
	T* find_component() const
	{
		auto predicate = [](const auto& value) { return typeid(*value) == typeid(T); };
		auto iterator = std::find_if(components.begin(), components.end(), predicate);
		return iterator == components.end() ? nullptr : static_cast<T*>(iterator->get());
	}

private:
    std::unique_ptr<RenderWindow> render_window;
    std::unique_ptr<ImGuiBackend> imgui_backend;

    std::vector<std::unique_ptr<Component>> components;
};
} // rw
