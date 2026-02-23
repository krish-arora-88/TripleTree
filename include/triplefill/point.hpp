#pragma once

#include <cstdint>
#include <functional>

namespace triplefill {

struct Point {
    int x = 0;
    int y = 0;

    constexpr Point() noexcept = default;
    constexpr Point(int x, int y) noexcept : x(x), y(y) {}
    constexpr bool operator==(const Point&) const noexcept = default;
};

} // namespace triplefill

template <>
struct std::hash<triplefill::Point> {
    std::size_t operator()(const triplefill::Point& p) const noexcept {
        auto h1 = std::hash<int>{}(p.x);
        auto h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 16);
    }
};
