#pragma once

#include <cstdint>

namespace triplefill {

struct RGBA {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr RGBA() noexcept = default;
    constexpr RGBA(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                   std::uint8_t a = 255) noexcept
        : r(r), g(g), b(b), a(a) {}

    constexpr bool operator==(const RGBA&) const noexcept = default;
};

} // namespace triplefill
