#pragma once

#include "Interface/Application/Component.hpp"
#include "Utility/BasicTypes.hpp"

namespace rw
{

class Application;

class GridView : public Component
{
public:
    explicit GridView(Application& application);
    ~GridView() override;

    void initialize() override;
    void update() override;

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

    // [[nodiscard]] const sf::RenderStates& get_render_states() const { return *render_states; }

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

    float change_zoom(float delta);

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
        update_grid();
    }

    // friend BinaryWriter& operator<<(BinaryWriter& writer, const LayerView& layer_view)
    // {
    //     return writer << layer_view.center << layer_view.zoom;
    // }
    //
    // friend BinaryReader& operator>>(BinaryReader& reader, LayerView& layer_view)
    // {
    //     reader >> layer_view.center >> layer_view.zoom;
    //     layer_view.mark_dirty();
    //     layer_view.update_zoom();
    //     return reader;
    // }

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
} // rw
