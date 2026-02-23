#pragma once

#include "pixel.hpp"
#include "point.hpp"

#include <cstdint>
#include <functional>
#include <variant>
#include <vector>

namespace triplefill {

// ---------------------------------------------------------------------------
// Picker configurations (value types — no heap, no virtual dispatch)
// ---------------------------------------------------------------------------

struct SolidPicker {
    RGBA color;
};

struct StripePicker {
    RGBA     color1;
    RGBA     color2;
    unsigned stripe_width = 10;
};

struct QuarterPicker {
    RGBA   color;
    int    bright = 40;   // luminance delta per quadrant
    Point  center{0, 0};  // image center; set by fill engine if {0,0}
};

struct BorderPicker {
    RGBA     fill_color;
    RGBA     border_color;
    unsigned border_width = 3;
};

using PickerConfig = std::variant<SolidPicker, StripePicker,
                                  QuarterPicker, BorderPicker>;

// ---------------------------------------------------------------------------
// Callable type used internally by the fill engine.
// Signature: (point, original_pixel_color) -> replacement color.
// ---------------------------------------------------------------------------
using ColorPickerFn = std::function<RGBA(Point, const RGBA&)>;

/// Build a concrete picker function from a config.
/// For BorderPicker the fill engine supplies a reference to the visited bitmap
/// and image dimensions so the picker can inspect neighbour fill state.
ColorPickerFn make_picker(const PickerConfig& cfg);
ColorPickerFn make_border_picker(const BorderPicker& bp,
                                 const std::vector<std::uint8_t>& visited,
                                 unsigned img_w, unsigned img_h);

} // namespace triplefill
