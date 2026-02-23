#include "triplefill/fill.hpp"
#include "triplefill/color_picker.hpp"
#include "triplefill/pickers/border.hpp"
#include "triplefill/pickers/quarter.hpp"
#include "triplefill/pickers/solid.hpp"
#include "triplefill/pickers/stripe.hpp"
#include "triplefill/tolerance.hpp"

#include <deque>
#include <vector>

namespace triplefill {

// ---- picker construction ---------------------------------------------------

ColorPickerFn make_picker(const PickerConfig& cfg) {
    return std::visit(
        [](const auto& p) -> ColorPickerFn {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, SolidPicker>) {
                return [p](Point pt, const RGBA& orig) {
                    return pick_solid(p, pt, orig);
                };
            } else if constexpr (std::is_same_v<T, StripePicker>) {
                return [p](Point pt, const RGBA& orig) {
                    return pick_stripe(p, pt, orig);
                };
            } else if constexpr (std::is_same_v<T, QuarterPicker>) {
                return [p](Point pt, const RGBA& orig) {
                    return pick_quarter(p, pt, orig);
                };
            } else { // BorderPicker -- dummy; real one built by fill engine
                return [c = p.fill_color](Point, const RGBA&) { return c; };
            }
        },
        cfg);
}

ColorPickerFn make_border_picker(const BorderPicker& bp,
                                 const std::vector<std::uint8_t>& visited,
                                 unsigned img_w, unsigned img_h) {
    return [&bp, &visited, img_w, img_h](Point pt, const RGBA& orig) {
        return pick_border(bp, visited, img_w, img_h, pt, orig);
    };
}

// ---- flood fill ------------------------------------------------------------

Animation flood_fill(const Image& img, const FillConfig& cfg) {
    const unsigned w = img.width();
    const unsigned h = img.height();

    Image canvas = img; // mutable copy
    Animation anim;

    if (w == 0 || h == 0) return anim;
    if (cfg.seed.x < 0 || cfg.seed.y < 0 ||
        static_cast<unsigned>(cfg.seed.x) >= w ||
        static_cast<unsigned>(cfg.seed.y) >= h)
        return anim;

    const auto npx = static_cast<std::size_t>(w) * h;
    std::vector<std::uint8_t> visited(npx, 0);

    const RGBA seed_color = img.at(static_cast<unsigned>(cfg.seed.x),
                                   static_cast<unsigned>(cfg.seed.y));

    // Construct picker — special-case BorderPicker
    ColorPickerFn picker;
    const bool is_border = std::holds_alternative<BorderPicker>(cfg.picker);
    if (is_border) {
        const auto& bp = std::get<BorderPicker>(cfg.picker);
        picker = make_border_picker(bp, visited, w, h);
    } else {
        PickerConfig adjusted = cfg.picker;
        // Auto-set QuarterPicker center if left at origin
        if (auto* qp = std::get_if<QuarterPicker>(&adjusted);
            qp && qp->center == Point{0, 0}) {
            qp->center = Point{static_cast<int>(w / 2),
                               static_cast<int>(h / 2)};
        }
        picker = make_picker(adjusted);
    }

    // Ordering structure: deque for BFS (FIFO), vector for DFS (LIFO)
    std::deque<Point>  bfs_queue;
    std::vector<Point> dfs_stack;

    auto push = [&](Point p) {
        if (cfg.algorithm == Algorithm::BFS)
            bfs_queue.push_back(p);
        else
            dfs_stack.push_back(p);
    };
    auto pop = [&]() -> Point {
        Point p;
        if (cfg.algorithm == Algorithm::BFS) {
            p = bfs_queue.front();
            bfs_queue.pop_front();
        } else {
            p = dfs_stack.back();
            dfs_stack.pop_back();
        }
        return p;
    };
    auto empty = [&]() -> bool {
        return cfg.algorithm == Algorithm::BFS
                   ? bfs_queue.empty()
                   : dfs_stack.empty();
    };
    auto queued = [&]() -> std::size_t {
        return cfg.algorithm == Algorithm::BFS
                   ? bfs_queue.size()
                   : dfs_stack.size();
    };

    // Mark visited on push
    auto mark = [&](int x, int y) {
        visited[static_cast<std::size_t>(y) * w + x] = 1;
    };
    auto is_visited = [&](int x, int y) -> bool {
        return visited[static_cast<std::size_t>(y) * w + x] != 0;
    };

    // Seed
    mark(cfg.seed.x, cfg.seed.y);
    push(cfg.seed);

    std::size_t filled = 0;
    const int freq = cfg.frame_freq;

    // Neighbour offsets: North, East, South, West
    static constexpr int dx[] = { 0, 1, 0, -1};
    static constexpr int dy[] = {-1, 0, 1,  0};

    while (!empty()) {
        const Point cur = pop();

        // Colour on pop
        const RGBA orig = canvas.at(static_cast<unsigned>(cur.x),
                                    static_cast<unsigned>(cur.y));
        canvas.at(static_cast<unsigned>(cur.x),
                  static_cast<unsigned>(cur.y)) = picker(cur, orig);

        ++filled;

        // Frame capture: every freq-th pixel, starting at the freq-th
        if (freq > 0 && (filled % static_cast<std::size_t>(freq)) == 0) {
            if (!cfg.max_frames || anim.size() < *cfg.max_frames)
                anim.add_frame(canvas);
        }

        if (cfg.on_progress)
            cfg.on_progress(filled, queued());

        // Push in-tolerance unvisited neighbours
        for (int d = 0; d < 4; ++d) {
            const int nx = cur.x + dx[d];
            const int ny = cur.y + dy[d];
            if (nx < 0 || ny < 0 ||
                static_cast<unsigned>(nx) >= w ||
                static_cast<unsigned>(ny) >= h)
                continue;
            if (is_visited(nx, ny)) continue;

            const RGBA& nc = img.at(static_cast<unsigned>(nx),
                                    static_cast<unsigned>(ny));
            if (color_distance(seed_color, nc) <= cfg.tolerance) {
                mark(nx, ny);
                push({nx, ny});
            }
        }
    }

    // Always add final frame
    anim.add_frame(canvas);

    return anim;
}

} // namespace triplefill
