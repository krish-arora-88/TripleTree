#include "triplefill/image.hpp"
#include "lodepng.h"

#include <stdexcept>

namespace triplefill {

Image load_png(const std::filesystem::path& path) {
    std::vector<unsigned char> raw;
    unsigned w = 0, h = 0;
    unsigned err = lodepng::decode(raw, w, h, path.string());
    if (err)
        throw std::runtime_error("PNG decode error (" + path.string() +
                                 "): " + lodepng_error_text(err));

    Image img(w, h);
    const std::size_t npx = static_cast<std::size_t>(w) * h;
    for (std::size_t i = 0; i < npx; ++i) {
        img.data()[i] = RGBA{raw[i * 4 + 0], raw[i * 4 + 1],
                             raw[i * 4 + 2], raw[i * 4 + 3]};
    }
    return img;
}

void save_png(const std::filesystem::path& path, const Image& img) {
    const std::size_t npx = img.pixel_count();
    std::vector<unsigned char> raw(npx * 4);
    for (std::size_t i = 0; i < npx; ++i) {
        const auto& px = img.data()[i];
        raw[i * 4 + 0] = px.r;
        raw[i * 4 + 1] = px.g;
        raw[i * 4 + 2] = px.b;
        raw[i * 4 + 3] = px.a;
    }
    unsigned err = lodepng::encode(path.string(), raw, img.width(), img.height());
    if (err)
        throw std::runtime_error("PNG encode error (" + path.string() +
                                 "): " + lodepng_error_text(err));
}

} // namespace triplefill
