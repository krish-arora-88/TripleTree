#include "triplefill/tolerance.hpp"

#include <algorithm>
#include <cmath>

namespace triplefill {

HSL rgb_to_hsl(const RGBA& c) noexcept {
    const double r = c.r / 255.0;
    const double g = c.g / 255.0;
    const double b = c.b / 255.0;

    const double mx = std::max({r, g, b});
    const double mn = std::min({r, g, b});
    const double chroma = mx - mn;

    HSL hsl{};
    hsl.l = 0.5 * (mx + mn);

    if (chroma < 1e-4 || mx < 1e-4) {
        hsl.h = 0.0;
        hsl.s = 0.0;
        return hsl;
    }

    hsl.s = chroma / (1.0 - std::fabs(2.0 * hsl.l - 1.0));

    if (mx == r)
        hsl.h = std::fmod((g - b) / chroma, 6.0);
    else if (mx == g)
        hsl.h = ((b - r) / chroma) + 2.0;
    else
        hsl.h = ((r - g) / chroma) + 4.0;

    hsl.h *= 60.0;
    if (hsl.h < 0.0) hsl.h += 360.0;

    return hsl;
}

RGBA hsl_to_rgb(const HSL& hsl, std::uint8_t a) noexcept {
    if (hsl.s <= 0.001) {
        auto v = static_cast<std::uint8_t>(std::clamp(
            std::round(hsl.l * 255.0), 0.0, 255.0));
        return {v, v, v, a};
    }

    const double c = (1.0 - std::fabs(2.0 * hsl.l - 1.0)) * hsl.s;
    const double hh = hsl.h / 60.0;
    const double x = c * (1.0 - std::fabs(std::fmod(hh, 2.0) - 1.0));
    double r = 0, g = 0, b = 0;

    if      (hh <= 1.0) { r = c; g = x; }
    else if (hh <= 2.0) { r = x; g = c; }
    else if (hh <= 3.0) { g = c; b = x; }
    else if (hh <= 4.0) { g = x; b = c; }
    else if (hh <= 5.0) { r = x; b = c; }
    else                { r = c; b = x; }

    const double m = hsl.l - 0.5 * c;
    auto clamp8 = [](double v) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::clamp(std::round(v * 255.0),
                                                    0.0, 255.0));
    };
    return {clamp8(r + m), clamp8(g + m), clamp8(b + m), a};
}

double color_distance(const RGBA& a, const RGBA& b) noexcept {
    const HSL ha = rgb_to_hsl(a);
    const HSL hb = rgb_to_hsl(b);

    double dh = std::fabs(ha.h - hb.h) / 360.0;
    if (dh > 0.5) dh = 1.0 - dh; // shortest arc

    const double ds = ha.s - hb.s;
    const double dl = ha.l - hb.l;

    return std::sqrt(dh * dh + ds * ds + dl * dl);
}

RGBA adjust_luminance(const RGBA& c, double delta) noexcept {
    HSL hsl = rgb_to_hsl(c);
    hsl.l = std::clamp(hsl.l + delta, 0.0, 1.0);
    return hsl_to_rgb(hsl, c.a);
}

} // namespace triplefill
