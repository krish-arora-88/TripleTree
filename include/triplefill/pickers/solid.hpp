#pragma once

#include "../color_picker.hpp"

namespace triplefill {

inline RGBA pick_solid(const SolidPicker& p, Point /*pt*/,
                       const RGBA& /*original*/) {
    return p.color;
}

} // namespace triplefill
