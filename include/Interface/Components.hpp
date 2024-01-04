#pragma once

#include "main.hpp"
#include "Application.hpp"
#include "Utility/SimpleTypes.hpp"
#include "Functional/Tiles.hpp"

#include <chrono>

namespace rw
{

class Controller : public Component
{
public:
	explicit Controller(Application& application);

	void initialize() override;
	void update() override;

	[[nodiscard]] Layer* get_layer() const { return layer.get(); }

private:
	enum class ActionType : uint8_t
	{
		None,
		Save,
		Load,
		New
	};

	void update_interface();

	std::unique_ptr<Layer> layer;
	std::array<char, 100> path_buffer{};

	ActionType selected_action = ActionType::None;
};

class LayerView : public Component
{
public:
	explicit LayerView(Application& application);
	~LayerView() override;

	void initialize() override;
	void update() override;
	void input_event(const sf::Event& event) override;

	[[nodiscard]] Float2 get_min() const { return center - extend; }

	[[nodiscard]] Float2 get_max() const { return center + extend; }

	[[nodiscard]] float get_aspect_ratio() const { return aspect_ratio; }

	[[nodiscard]]
	Float2 get_point(Float2 percent) const
	{
		percent = percent * 2.0f - Float2(1.0f);
		return center + extend * percent;
	}

	[[nodiscard]] const sf::RenderStates& get_render_states() const { return *render_states; }

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

		mark_dirty();
		update_zoom();
		return zoom;
	}

	float change_zoom(float delta, Float2 percent)
	{
		Float2 point = get_point(percent);
		float result = change_zoom(delta);
		set_point(percent, point);
		return result;
	}

	void reset()
	{
		center = Float2(0.0f);
		zoom = 1.7f;
		mark_dirty();
		update_zoom();
	}

	friend BinaryWriter& operator<<(BinaryWriter& writer, const LayerView& layer_view)
	{
		return writer << layer_view.center << layer_view.zoom;
	}

	friend BinaryReader& operator>>(BinaryReader& reader, LayerView& layer_view)
	{
		reader >> layer_view.center >> layer_view.zoom;
		layer_view.mark_dirty();
		layer_view.update_zoom();
		return reader;
	}

private:
	void get_scale_origin(float& scale, Float2& origin) const;

	void mark_dirty() { dirty = true; }

	void update_zoom();
	void update_grid();
	void update_render_states();

	void draw_grid() const;
	void draw_layer(const Layer& layer) const;

	Controller* controller{};

	Float2 center;
	Float2 extend;
	float aspect_ratio{};
	bool dirty = true;

	float zoom{};
	int32_t zoom_level{};
	int32_t zoom_gap{};
	float zoom_scale{};
	float zoom_percent{};

	std::vector<sf::Vertex> vertices;
	std::unique_ptr<sf::Shader> shader_quad;
	std::unique_ptr<sf::Shader> shader_wire;
	std::unique_ptr<DrawContext> draw_context;
	std::unique_ptr<sf::RenderStates> render_states;

	static constexpr int32_t ZoomIncrement = 8;
	static constexpr float ZoomLevelShift = 0.7f;
	static constexpr float GridLineAlpha = 45.0f;
};

class Cursor : public Component
{
public:
	explicit Cursor(Application& application);

	void initialize() override;
	void update() override;
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
	void update_key_event(const sf::Event& event);
	void update_mouse_event(const sf::Event& event);

	void execute_key();
	void execute_mouse(Int2 position);
	Int2 place_wire(Int2 position);
	Int2 place_port(Int2 position);

	void draw_rectangle(Float2 center, Float2 size, uint32_t fill_color = ~uint32_t(), float outline = 0.0f, uint32_t outline_color = ~uint32_t());

	Controller* controller{};
	LayerView* layer_view{};

	Float2 mouse_percent;
	Float2 last_mouse_point;

	float selected_pan_sensitivity = 0.5f;
	ToolType selected_tool = ToolType::Mouse;
	PortType selected_port = PortType::Transistor;
	bool selected_auto_bridge = false;
	TileRotation selected_rotation;

	DragType drag_type = DragType::None;
	Int2 drag_origin;

	std::unique_ptr<sf::RectangleShape> rectangle;
};

class TickControl : public Component
{
public:
	explicit TickControl(Application& application);

	void initialize() override;
	void update() override;
	void input_event(const sf::Event& event) override;

	void pause()
	{
		selected_pause = true;
		update_display();
	}

	void resume() { selected_pause = false; }

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

	struct TicksPair
	{
		TicksPair() = default;

		TicksPair(Duration duration, uint32_t count) : duration(duration), count(count) {}

		TicksPair& operator+=(TicksPair other)
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
	void update(Engine& engine);

	uint32_t execute(Engine& engine, uint64_t count);

	void begin_manual()
	{
		remain_count = selected_count;
		executed = {};
		update_display();
	}

	static constexpr Duration as_duration(uint32_t milliseconds)
	{
		auto result = std::chrono::milliseconds(milliseconds);
		return std::chrono::duration_cast<Duration>(result);
	}

	Controller* controller{};

	Type selected_type = Type::PerSecond;
	uint64_t selected_count = 32;
	bool selected_pause = false;

	float last_display_time = 0.0f;
	uint64_t last_frame_count = 0;
	std::string display_frames_per_second;
	std::string display_ticks_per_second;
	std::string display_dropped_ticks;

	uint64_t remain_count{};
	uint64_t dropped_count = 0;
	float per_second_error = 0.0f;

	Duration time_budget = as_duration(10);
	Duration last_execute_rate = time_budget / 10;
	TicksPair executed;
};

class Debugger : public Component
{
public:
	explicit Debugger(Application& application);

	void initialize() override {}

	void update() override;
};

}
