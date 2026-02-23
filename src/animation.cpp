#include "triplefill/animation.hpp"
#include "gif.h"

#include <stdexcept>

namespace triplefill {

void Animation::write_last_png(const std::filesystem::path& path) const {
    if (frames_.empty())
        throw std::runtime_error("Animation has no frames");
    save_png(path, frames_.back());
}

void Animation::write_gif(const std::filesystem::path& path,
                          unsigned delay_cs) const {
    if (frames_.empty())
        throw std::runtime_error("Animation has no frames");

    const unsigned w = frames_[0].width();
    const unsigned h = frames_[0].height();

    GifWriter gw;
    if (!gif_begin(&gw, path.string().c_str(),
                   static_cast<int>(w), static_cast<int>(h),
                   static_cast<uint16_t>(delay_cs)))
        throw std::runtime_error("Cannot open GIF file: " + path.string());

    for (const auto& frame : frames_) {
        if (!gif_write_frame(&gw,
                             reinterpret_cast<const uint8_t*>(frame.data()),
                             static_cast<int>(w), static_cast<int>(h),
                             static_cast<uint16_t>(delay_cs))) {
            gif_end(&gw);
            throw std::runtime_error("Failed writing GIF frame");
        }
    }

    gif_end(&gw);
}

} // namespace triplefill
