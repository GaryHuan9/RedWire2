#pragma once

#include "main.hpp"

#include <utility>
#include <algorithm>
#include <stdexcept>

class GLFWwindow;

namespace rw
{

class Application
{
public:
	Application();
	~Application();

	void run();

	[[nodiscard]] RenderWindow& get_window() const { return *window; }

	template<class T>
	[[nodiscard]] T* find_component() const
	{
		auto predicate = [](const auto& value) { return typeid(*value) == typeid(T); };
		auto iterator = std::find_if(components.begin(), components.end(), predicate);
		return iterator == components.end() ? nullptr : static_cast<T*>(iterator->get());
	}

	[[nodiscard]] const ShaderResources& get_shaders() const { return *shader_resources; }

	[[nodiscard]] const Timer& get_timer() const { return *timer; }

	[[nodiscard]] bool handle_mouse() const;
	[[nodiscard]] bool handle_keyboard() const;

	template<class T, class... Arguments>
	T* make_component(Arguments&& ... arguments)
	{
		auto predicate = [](const auto& value) { return typeid(*value) == typeid(T); };
		size_t count = std::count_if(components.begin(), components.end(), predicate);
		if (count != 0) throw std::invalid_argument("Already added component.");

		auto component = std::make_unique<T>(*this, std::forward<Arguments>(arguments)...);
		T* pointer = static_cast<T*>(component.get());
		components.emplace_back(std::move(component));
		return pointer;
	}

private:
//	void process_event(const sf::Event& event, bool& closed);

	std::unique_ptr<RenderWindow> window{};
	std::vector<std::unique_ptr<Component>> components;
	std::unique_ptr<ShaderResources> shader_resources;
	std::unique_ptr<Timer> timer;
};

class Component
{
public:
	explicit Component(Application& application);

	virtual ~Component() = default;

	virtual void initialize() = 0;

	virtual void update() = 0;

//	virtual void input_event(const sf::Event& event) {}

protected:
	Application& application;
};

class Timer
{
public:
	[[nodiscard]]
	uint64_t frame_time() const
	{
		return last_delta_time;
	}

	[[nodiscard]]
	uint64_t time() const
	{
		return update_time;
	}

	[[nodiscard]]
	uint64_t frame_count() const
	{
		return update_count;
	}

	void update(uint64_t delta_time)
	{
		last_delta_time = delta_time;
		update_time += delta_time;
		++update_count;
	}

	static float as_float(uint64_t time)
	{
		return static_cast<float>(time) * 1.0E-6f;
	}

private:
	uint64_t last_delta_time{};
	uint64_t update_time{};
	uint64_t update_count{};
};

class RenderWindow
{
public:
	RenderWindow(Int2 size, const std::string& name);
	~RenderWindow();

	void initialize() const;
	void draw() const;

	[[nodiscard]] Int2 get_size() const;
	[[nodiscard]] bool is_focused() const;

	[[nodiscard]] bool next_iteration() const;

private:

	[[nodiscard]] bool get_attribute(int attribute) const;

	GLFWwindow* pointer{};
};

}
