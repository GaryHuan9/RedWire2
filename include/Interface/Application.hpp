#pragma once

#include "main.hpp"

#include <utility>
#include <algorithm>
#include <stdexcept>

namespace rw
{

class Application
{
public:
	Application();
	~Application();

	void run();

	template<typename T>
	T* find_component() const
	{
		auto predicate = [](const auto& value) { return typeid(*value) == typeid(T); };
		auto iterator = std::ranges::find_if(components, predicate);
		return iterator == components.end() ? nullptr : static_cast<T*>(iterator->get());
	}

	template<typename T, typename... Arguments>
	T* make_component(Arguments&& ... arguments)
	{
		auto predicate = [](const auto& value) { return typeid(*value) == typeid(T); };
		size_t count = std::ranges::count_if(components, predicate);
		if (count != 0) throw std::invalid_argument("Already added component.");

		auto component = std::make_unique<T>(*this, std::forward<Arguments>(arguments)...);
		T* pointer = static_cast<T*>(component.get());
		components.emplace_back(std::move(component));
		return pointer;
	}

	[[nodiscard]] sf::RenderWindow* get_window() const
	{
		return window.get();
	}

	[[nodiscard]] static bool capture_mouse();
	[[nodiscard]] static bool capture_keyboard();

private:
	void process_event(const sf::Event& event);

	std::unique_ptr<sf::RenderWindow> window;
	std::vector<std::unique_ptr<Component>> components;
};

class Component
{
public:
	explicit Component(Application& application);

	virtual ~Component() = default;

	virtual void initialize() = 0;

	virtual void update(const Timer& timer) = 0;

	virtual void input_event(const sf::Event& event) {}

protected:
	Application& application;
	sf::RenderWindow& window;
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

}
