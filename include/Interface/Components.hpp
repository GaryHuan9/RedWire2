#pragma once

#include "main.hpp"
#include "Application.hpp"
#include "Utility/Types.hpp"
#include "Functional/Tiles.hpp"

#include <chrono>

namespace rw
{

class Controller : public Component
{
public:
	explicit Controller(Application& application);

	void initialize() override;
	void update(const Timer& timer) override;

	[[nodiscard]] Layer* get_layer() const { return layer.get(); }

private:
	std::unique_ptr<Layer> layer;
};

class LayerView : public Component
{
public:
	explicit LayerView(Application& application);
	~LayerView() override;

	void initialize() override;
	void update(const Timer& timer) override;
	void input_event(const sf::Event& event) override;

	[[nodiscard]] Float2 get_min() const { return center - extend; }

	[[nodiscard]] Float2 get_max() const { return center + extend; }

	[[nodiscard]] Float2 get_point(Float2 percent) const;

	[[nodiscard]] sf::RenderStates get_render_states() const;

	void set_aspect_ratio(float value)
	{
		if (value == aspect_ratio) return;
		aspect_ratio = value;

		mark_dirty();
		update_zoom();
	}

	void set_point(Float2 percent, Float2 point)
	{
		percent = percent * 2.0f - Float2(1.0f);
		center = point - extend * percent;

		mark_dirty();
	}

	float change_zoom(float delta)
	{
		float new_zoom = std::clamp(zoom + delta, 0.0f, 5.0f);
		if (new_zoom == zoom) return zoom;
		zoom = new_zoom;

		update_zoom();
		mark_dirty();
		return zoom;
	}

	float change_zoom(float delta, Float2 percent)
	{
		Float2 point = get_point(percent);
		float result = change_zoom(delta);
		set_point(percent, point);
		return result;
	}

private:
	void get_scale_origin(float& scale, Float2& origin) const;

	void mark_dirty() { dirty = true; }

	void update_zoom();
	void update_grid();

	void draw_grid() const;
	void draw_layer(const Layer& layer) const;

	Controller* controller{};

	Float2 center;
	Float2 extend;
	float aspect_ratio{};
	bool dirty = true;

	float zoom = 1.7f;
	int32_t zoom_level{};
	int32_t zoom_gap{};
	float zoom_scale{};
	float zoom_percent{};

	std::vector<sf::Vertex> vertices;
	std::unique_ptr<sf::Shader> shader_quad;
	std::unique_ptr<sf::Shader> shader_wire;
	std::unique_ptr<DrawContext> draw_context;

	static constexpr int32_t ZoomIncrement = 8;
	static constexpr float ZoomLevelShift = 0.7f;
	static constexpr float GridLineAlpha = 45.0f;
};

class Cursor : public Component
{
public:
	explicit Cursor(Application& application);

	void initialize() override;
	void update(const Timer& timer) override;
	void input_event(const sf::Event& event) override;

	bool try_get_mouse_position(Int2& position) const;

private:
	enum class ToolType : uint8_t
	{
		Mouse,
		WirePlacement,
		PortPlacement,
		TileRemoval
	};

	enum class PortType : uint8_t
	{
		Transistor,
		Inverter,
		Bridge
	};

	enum class DragType : uint8_t
	{
		None,
		Origin,
		Vertical,
		Horizontal
	};

	void update_interface();
	void update_input_event(const sf::Event& event);

	void execute(Int2 position);
	void execute_drag(Int2 position);
	Int2 place_wire(Int2 position);
	Int2 place_port(Int2 position);

	Controller* controller{};
	LayerView* layer_view{};

	Float2 mouse_percent;
	Float2 mouse_delta;

	ToolType selected_tool = ToolType::Mouse;
	PortType selected_port = PortType::Transistor;
	TileRotation selected_rotation;

	DragType drag_type = DragType::None;
	Int2 drag_origin;

	std::unique_ptr<sf::RectangleShape> shape_outline;
	std::unique_ptr<sf::RectangleShape> shape_fill;
};

class Updater : public Component
{
public:
	explicit Updater(Application& application);

	void initialize() override;
	void update(const Timer& timer) override;

	void pause()
	{
		selected_pause = true;
		update_display();
	}

	void resume()
	{
		selected_pause = false;
	}

private:
	using Clock = std::chrono::high_resolution_clock;
	using Duration = std::chrono::time_point<Clock>::duration;

	enum class Type : uint8_t
	{
		PerSecond,
		PerFrame,
		Manual,
		Maximum
	};

	struct ExecutePair
	{
		ExecutePair() = default;

		ExecutePair(Duration duration, uint32_t count) : duration(duration), count(count) {}

		ExecutePair& operator+=(ExecutePair other)
		{
			duration += other.duration;
			count += other.count;
			return *this;
		}

		Duration duration{};
		uint64_t count = 0;
	};

	void update_interface();
	void update_display();
	void update(float delta_time, Engine& engine);

	uint32_t execute(Engine& engine, uint64_t count);

	static constexpr Duration as_duration(uint32_t milliseconds)
	{
		auto result = std::chrono::milliseconds(milliseconds);
		return std::chrono::duration_cast<Duration>(result);
	}

	Controller* controller{};

	Type selected_type = Type::PerSecond;
	uint64_t selected_count = 10;
	bool selected_pause = false;

	float last_display_time = 0.0f;
	std::string display_updates_per_second;
	std::string display_dropped_update_count;

	uint64_t remain_count{};
	uint64_t dropped_count = 0;
	float per_second_error = 0.0f;

	Duration time_budget = as_duration(10);
	Duration last_execute_rate = time_budget / 10;
	ExecutePair executed;
};

class Debugger : public Component
{
public:
	explicit Debugger(Application& application);

	void initialize() override {}

	void update(const Timer& timer) override;
};

}
