#pragma once

#include "image.hpp"

#include <filesystem>
#include <vector>

namespace triplefill {

class Animation {
public:
    Animation() = default;

    void reserve(std::size_t n) { frames_.reserve(n); }
    void add_frame(Image img) { frames_.push_back(std::move(img)); }

    [[nodiscard]] std::size_t   size()  const noexcept { return frames_.size(); }
    [[nodiscard]] bool          empty() const noexcept { return frames_.empty(); }

    [[nodiscard]] const Image& frame(std::size_t i) const { return frames_.at(i); }
    [[nodiscard]] const Image& final_frame()        const { return frames_.back(); }

    [[nodiscard]] const std::vector<Image>& frames() const noexcept {
        return frames_;
    }

    void write_last_png(const std::filesystem::path& path) const;
    void write_gif(const std::filesystem::path& path,
                   unsigned delay_cs = 4) const;

private:
    std::vector<Image> frames_;
};

} // namespace triplefill
