#include "triplefill/pickers/border.hpp"

namespace triplefill {

RGBA pick_border(const BorderPicker& bp,
                 const std::vector<std::uint8_t>& visited,
                 unsigned img_w, unsigned img_h,
                 Point pt, const RGBA& /*original*/) {
    const int bw = static_cast<int>(bp.border_width);
    const int w  = static_cast<int>(img_w);
    const int h  = static_cast<int>(img_h);

    auto is_filled = [&](int x, int y) -> bool {
        if (x < 0 || x >= w || y < 0 || y >= h) return false;
        return visited[static_cast<std::size_t>(y) * img_w + x] != 0;
    };

    // Check all four cardinal directions up to border_width steps.
    for (int d = 1; d <= bw; ++d) {
        if (!is_filled(pt.x,     pt.y - d)) return bp.border_color; // N
        if (!is_filled(pt.x + d, pt.y))     return bp.border_color; // E
        if (!is_filled(pt.x,     pt.y + d)) return bp.border_color; // S
        if (!is_filled(pt.x - d, pt.y))     return bp.border_color; // W
    }

    return bp.fill_color;
}

} // namespace triplefill
