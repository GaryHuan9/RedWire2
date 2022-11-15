#pragma once

#include "main.hpp"
#include "Utility/Vector2.hpp"

namespace sf
{
	class RenderWindow;
	class Vertex;
}

namespace rw
{

class LayerView
{
public:
	explicit LayerView(float aspect_ratio, float zoom = 1.7f);

	void set_aspect_ratio(float value)
	{
		if (value == aspect_ratio) return;
		aspect_ratio = value;
		update_zoom();
	}

	[[nodiscard]]
	Float2 get_min() const { return center - extend; }

	[[nodiscard]]
	Float2 get_max() const { return center + extend; }

	[[nodiscard]]
	Float2 get_point(Float2 percent) const
	{
		percent = percent * 2.0f - Float2(1.0f);
		return center + extend * percent;
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

	void draw(sf::RenderWindow& window) const;

private:
	void update_zoom();

	Float2 center;
	Float2 extend;
	float aspect_ratio;

	float zoom;
	int32_t zoom_level{};
	int32_t zoom_gap{};
	float zoom_scale{};
	float zoom_percent{};

	mutable std::vector<sf::Vertex> vertices;

	static constexpr int32_t ZoomIncrement = 8;
	static constexpr float ZoomLevelShift = 0.7f;
	static constexpr float GridLineAlpha = 45.0f;
};

} // rw2
