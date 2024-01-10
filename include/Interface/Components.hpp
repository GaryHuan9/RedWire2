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

	[[nodiscard]] Float2 get_center() const { return center; }

	[[nodiscard]] Float2 get_extend() const { return extend; }

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
	void get_scale_origin(Float2& scale, Float2& origin) const;

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

	bool try_get_mouse_position(Int2& position) const
	{
		if (not application.handle_mouse()) return false;
		position = Float2::floor(mouse_point);
		return true;
	}

private:
	class Tool;
	class MouseTool;
	class WireTool;
	class DeviceTool;
	class RemovalTool;
	class ClipboardTool;

	void update_interface();
	void update_panning();

	Controller* controller{};
	LayerView* layer_view{};
	std::vector<std::unique_ptr<Tool>> tools;

	Float2 mouse_percent;
	Float2 mouse_point;
	Float2 last_mouse_point;

	float selected_pan_sensitivity = 0.5f;
	uint32_t selected_tool = 0;
};

class Cursor::Tool
{
public:
	explicit Tool(const Cursor& cursor) : cursor(cursor) {}

	virtual ~Tool() = default;

	void update_mouse();
	void deactivate() { drag_type = DragType::None; }

	virtual void update_interface() = 0;
	virtual bool request_activation(const sf::Event& event) = 0;
	virtual void input_event(const sf::Event& event) {};

protected:
	enum class DragType : uint8_t
	{
		None,
		Free,
		Vertical,
		Horizontal
	};

	[[nodiscard]] virtual Int2 get_placement_size() const { return Int2(1); }
	[[nodiscard]] virtual bool restrict_drag_axis() const { return true; }
	[[nodiscard]] bool mouse_pressed() const { return drag_type != DragType::None; }

	virtual void update(Int2 position) = 0;
	virtual void commit(Layer& layer) = 0;

	void draw_rectangle(Float2 center, Float2 extend, uint32_t color);
	void draw_selection(Bounds bounds, uint32_t color);
	void draw_removal(Bounds bounds);

	DragType drag_type = DragType::None;
	Int2 drag_origin;
	Int2 drag_position;

private:
	const Cursor& cursor;
};

class Cursor::MouseTool : public Tool
{
public:
	explicit MouseTool(Cursor& cursor) : Tool(cursor), cursor(cursor) {}

	void update_interface() override {}
	bool request_activation(const sf::Event& event) override;

protected:
	void update(Int2 position) override;
	void commit(Layer& layer) override {}

private:
	Cursor& cursor;
};

class Cursor::WireTool : public Tool
{
public:
	explicit WireTool(const Cursor& cursor) : Tool(cursor) {}

	void update_interface() override;
	bool request_activation(const sf::Event& event) override;

protected:
	void update(Int2 position) override;
	void commit(Layer& layer) override;

private:
	bool selected_auto_bridge = false;
};

class Cursor::DeviceTool : public Tool
{
public:
	explicit DeviceTool(const Cursor& cursor) : Tool(cursor) {}

	void update_interface() override;
	bool request_activation(const sf::Event& event) override;
	void input_event(const sf::Event& event) override;

protected:
	void update(Int2 position) override;
	void commit(Layer& layer) override;

private:
	enum class Type : uint8_t
	{
		Transistor,
		Inverter,
		Bridge
	};

	Type selected_type = Type::Transistor;
	TileRotation selected_rotation;
};

class Cursor::RemovalTool : public Tool
{
public:
	explicit RemovalTool(const Cursor& cursor) : Tool(cursor) {}

	void update_interface() override {}
	bool request_activation(const sf::Event& event) override;

protected:
	[[nodiscard]] bool restrict_drag_axis() const override { return false; }

	void update(Int2 position) override;
	void commit(Layer& layer) override;
};

class Cursor::ClipboardTool : public Tool
{
public:
	explicit ClipboardTool(const Cursor& cursor);

	void update_interface() override;
	bool request_activation(const sf::Event& event) override;
	void input_event(const sf::Event& event) override;

protected:
	[[nodiscard]] Int2 get_placement_size() const override;
	[[nodiscard]] bool restrict_drag_axis() const override;

	void update(Int2 position) override;
	void commit(Layer& layer) override;

private:
	enum class Type : uint8_t
	{
		Cut,
		Copy,
		Paste
	};

	class Buffer;

	LayerView& layer_view;
	std::unique_ptr<DrawContext> draw_context;

	Type selected_type = Type::Copy;
	std::unique_ptr<Buffer> buffer;
};

class Cursor::ClipboardTool::Buffer
{
public:
	Buffer(const Layer& source, Bounds bounds);

	[[nodiscard]] Int2 size() const
	{
		Int2 result = bounds.size();
		if (rotation.vertical()) std::swap(result.x, result.y);
		return result;
	}

	[[nodiscard]] TileRotation get_rotation() const { return rotation; }

	[[nodiscard]] Int2 get_position(Float2 center) const
	{
		Float2 offset = Float2(size() - Int2(1)) / 2.0;
		return Float2::floor(center - offset);
	}

	void set_rotation(TileRotation new_rotation) { rotation = new_rotation; }

	void paste(Int2 position, Layer& layer) const;
	void draw(Int2 position, DrawContext& context, const LayerView& layer_view) const;

private:
	std::unique_ptr<Layer> buffer;
	const Bounds bounds;
	TileRotation rotation;
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
