#pragma once

#include "pixel.hpp"

namespace triplefill {

struct HSL {
    double h = 0.0; // hue in [0, 360)
    double s = 0.0; // saturation in [0, 1]
    double l = 0.0; // lightness in [0, 1]
};

HSL  rgb_to_hsl(const RGBA& c) noexcept;
RGBA hsl_to_rgb(const HSL& c, std::uint8_t a = 255) noexcept;

/// Euclidean distance in normalised HSL space.
/// Hue is mapped to [0, 1] via h/360, shortest-arc distance is used.
/// Result is sqrt(dh^2 + ds^2 + dl^2), range roughly [0, ~1.22].
double color_distance(const RGBA& a, const RGBA& b) noexcept;

/// Clamp-add brightness delta to an RGBA color (operates in HSL space).
RGBA adjust_luminance(const RGBA& c, double delta) noexcept;

} // namespace triplefill
