#pragma once

#include "animation.hpp"
#include "color_picker.hpp"
#include "image.hpp"
#include "point.hpp"

#include <cstddef>
#include <functional>
#include <optional>

namespace triplefill {

enum class Algorithm { BFS, DFS };

/// Optional progress callback: (pixels_filled, pixels_queued).
using ProgressFn = std::function<void(std::size_t, std::size_t)>;

struct FillConfig {
    Point          seed{0, 0};
    double         tolerance    = 0.1;
    int            frame_freq   = 1000;
    Algorithm      algorithm    = Algorithm::BFS;
    PickerConfig   picker       = SolidPicker{RGBA{255, 0, 0}};
    std::optional<std::size_t> max_frames{};
    ProgressFn     on_progress{};
};

/// Run flood fill on a *copy* of `img` and return an Animation of frames.
/// The final frame is always appended (the completed fill result).
///
/// Neighbour push order: North, East, South, West.
/// Pixels are marked visited on push; coloured on pop.
Animation flood_fill(const Image& img, const FillConfig& cfg);

} // namespace triplefill
