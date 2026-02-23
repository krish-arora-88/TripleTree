#pragma once

#include "../color_picker.hpp"

#include <cstdint>
#include <vector>

namespace triplefill {

/// Returns border_color when any pixel within `border_width` steps
/// in any cardinal direction is not part of the filled region; otherwise
/// returns fill_color.
RGBA pick_border(const BorderPicker& bp,
                 const std::vector<std::uint8_t>& visited,
                 unsigned img_w, unsigned img_h,
                 Point pt, const RGBA& original);

} // namespace triplefill
