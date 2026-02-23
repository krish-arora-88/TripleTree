#pragma once

#include "../color_picker.hpp"
#include "../tolerance.hpp"

namespace triplefill {

/// Divides the image into four quadrants around `center`.
/// Each quadrant applies a cumulative luminance shift:
///   top-left  = color,
///   top-right = color + bright,
///   bot-left  = color + 2*bright,
///   bot-right = color + 3*bright.
RGBA pick_quarter(const QuarterPicker& p, Point pt, const RGBA& original);

} // namespace triplefill
