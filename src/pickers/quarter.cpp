#include "triplefill/pickers/quarter.hpp"

namespace triplefill {

RGBA pick_quarter(const QuarterPicker& p, Point pt,
                  const RGBA& /*original*/) {
    const bool right = pt.x >= p.center.x;
    const bool below = pt.y >= p.center.y;

    // Quadrant index: TL=0, TR=1, BL=2, BR=3
    const int quad = (right ? 1 : 0) + (below ? 2 : 0);
    const double delta = (quad * p.bright) / 255.0;
    return adjust_luminance(p.color, delta);
}

} // namespace triplefill
