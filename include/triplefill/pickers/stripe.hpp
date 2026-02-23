#pragma once

#include "../color_picker.hpp"

namespace triplefill {

/// Diagonal forward-slash stripes:
/// stripe index = (x + y) / stripe_width
/// even index -> color1, odd index -> color2
inline RGBA pick_stripe(const StripePicker& p, Point pt,
                        const RGBA& /*original*/) {
    const unsigned idx =
        static_cast<unsigned>(pt.x + pt.y) / p.stripe_width;
    return (idx % 2 == 0) ? p.color1 : p.color2;
}

} // namespace triplefill
