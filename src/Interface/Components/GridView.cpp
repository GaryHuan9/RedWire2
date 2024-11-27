#include "GridView.hpp"

#include <algorithm>

namespace rw
{
float GridView::change_zoom(float delta)
{
    float new_zoom = std::clamp(zoom + delta, 0.0f, 5.0f);
    if (new_zoom == zoom) return zoom;
    zoom = new_zoom;

    mark_dirty();
    update_zoom();
    return zoom;
}
} // rw
