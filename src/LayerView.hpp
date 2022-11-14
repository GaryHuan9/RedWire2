#pragma once

#include "main.hpp"
#include "Vector2.hpp"

namespace rw
{

class LayerView
{
public:
	explicit LayerView(float aspect_ratio, float zoom = 1.7f)
		: aspect_ratio(aspect_ratio), zoom(zoom) { update_zoom(); }

	[[nodiscard]]
	Float2 get_min() const { return center - extend; }

	[[nodiscard]]
	Float2 get_max() const { return center + extend; }

	void set_aspect_ratio(float value)
	{
		if (value == aspect_ratio) return;
		aspect_ratio = value;
		update_zoom();
	}

	Float2 change_center(Float2 delta) { return center += delta; }

	float change_zoom(float delta)
	{
		float new_zoom = std::clamp(zoom + delta, 0.0f, 5.0f);
		if (new_zoom == zoom) return zoom;

		zoom = new_zoom;
		update_zoom();
		return zoom;
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
	static constexpr float GridLineAlpha = 50.0f;
};

} // rw2
