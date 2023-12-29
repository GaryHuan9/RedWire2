#pragma once

#include "main.hpp"
#include "Application.hpp"
#include "Utility/Types.hpp"
#include "Functional/Tiles.hpp"

#include <optional>

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
		update_zoom();
	}

	void set_point(Float2 percent, Float2 point)
	{
		percent = percent * 2.0f - Float2(1.0f);
		center = point - extend * percent;
	}

	float change_zoom(float delta)
	{
		float new_zoom = std::clamp(zoom + delta, 0.0f, 5.0f);
		if (new_zoom == zoom) return zoom;

		zoom = new_zoom;
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

private:
	void update_zoom();

	void get_scale_origin(float& scale, Float2& origin) const;

	void draw_grid() const;
	void draw_layer(const Layer& layer) const;

	Controller* controller{};

	Float2 center;
	Float2 extend;
	float aspect_ratio{};

	float zoom = 1.7f;
	int32_t zoom_level{};
	int32_t zoom_gap{};
	float zoom_scale{};
	float zoom_percent{};

	mutable std::vector<sf::Vertex> vertices;
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

	ToolType current_tool = ToolType::Mouse;
	PortType current_port = PortType::Transistor;
	TileRotation current_rotation;

	DragType drag_type = DragType::None;
	Int2 drag_origin;

	std::unique_ptr<sf::RectangleShape> shape_outline;
	std::unique_ptr<sf::RectangleShape> shape_fill;

	static constexpr std::array ToolNames = { "Mouse", "Wire Placement", "Port Placement", "Tile Removal" };
	static constexpr std::array PortNames = { "Transistor", "Inverter", "Bridge" };
};

class Debugger : public Component
{
public:
	explicit Debugger(Application& application);

	void initialize() override {}

	void update(const Timer& timer) override;
};

}
