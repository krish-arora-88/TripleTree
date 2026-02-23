#pragma once

#include "pixel.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace triplefill {

class Image {
public:
    Image() noexcept = default;

    Image(unsigned w, unsigned h)
        : w_(w), h_(h), pixels_(static_cast<std::size_t>(w) * h) {}

    Image(unsigned w, unsigned h, RGBA fill)
        : w_(w), h_(h), pixels_(static_cast<std::size_t>(w) * h, fill) {}

    [[nodiscard]] unsigned width()  const noexcept { return w_; }
    [[nodiscard]] unsigned height() const noexcept { return h_; }
    [[nodiscard]] bool     empty()  const noexcept { return pixels_.empty(); }

    RGBA& at(unsigned x, unsigned y) {
        assert(x < w_ && y < h_);
        return pixels_[static_cast<std::size_t>(y) * w_ + x];
    }

    const RGBA& at(unsigned x, unsigned y) const {
        assert(x < w_ && y < h_);
        return pixels_[static_cast<std::size_t>(y) * w_ + x];
    }

    RGBA*       data()       noexcept { return pixels_.data(); }
    const RGBA* data() const noexcept { return pixels_.data(); }

    [[nodiscard]] std::size_t pixel_count() const noexcept {
        return pixels_.size();
    }

    bool operator==(const Image&) const = default;

private:
    unsigned w_ = 0;
    unsigned h_ = 0;
    std::vector<RGBA> pixels_;
};

[[nodiscard]] Image load_png(const std::filesystem::path& path);
void save_png(const std::filesystem::path& path, const Image& img);

} // namespace triplefill
