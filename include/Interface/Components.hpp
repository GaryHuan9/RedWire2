#pragma once

#include "main.hpp"
#include "Application.hpp"
#include "Utility/Types.hpp"
#include "Core/Tiles.hpp"

namespace rw
{

class LayerView : public Component
{
public:
	explicit LayerView(Application& application);
	~LayerView() override;

	void initialize() override
	{

	}

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

	void set_current_layer(Layer* new_current_layer)
	{
		current_layer = new_current_layer;
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

	void draw_grid(sf::RenderWindow& window) const;
	void draw_layer(sf::RenderWindow& window, const Layer& layer) const;

	Float2 center;
	Float2 extend;
	float aspect_ratio{};
	Layer* current_layer{};

	float zoom = 1.7f;
	int32_t zoom_level{};
	int32_t zoom_gap{};
	float zoom_scale{};
	float zoom_percent{};

	mutable std::vector<sf::Vertex> vertices;

	static constexpr int32_t ZoomIncrement = 8;
	static constexpr float ZoomLevelShift = 0.7f;
	static constexpr float GridLineAlpha = 45.0f;
};

class Controller : public Component
{
public:
	explicit Controller(Application& application);

	void initialize() override;
	void update(const Timer& timer) override;
	void input_event(const sf::Event& event) override;

	[[nodiscard]] Layer* get_layer() const { return layer.get(); }

	bool try_get_mouse_position(Int2& position) const;

private:
	std::unique_ptr<Layer> layer;
	LayerView* layer_view{};

	Float2 mouse_percent;
	Float2 mouse_delta;
	uint32_t selected_tool = 0;
	TileRotation rotation;
};

class Debugger : public Component
{
public:
	explicit Debugger(Application& application);

	void initialize() override;
	void update(const Timer& timer) override;

private:
	LayerView* layer_view{};
	Controller* controller{};
};

}
