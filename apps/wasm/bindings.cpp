// WASM bindings for triplefill — thin C ABI over the C++ library.
// Compiled with Emscripten; consumed by the web frontend worker.
//
// Every extern "C" function is wrapped in try/catch so that C++ exceptions
// (e.g. std::bad_alloc from large images) return error codes instead of
// calling abort().
//
// maxFrames/frameFreq contract:
//   frameFreq == 0 → final-only (1 frame, no intermediates)
//   maxFrames == 0 → unlimited intermediate frames (nullopt)
//   maxFrames > 0  → cap intermediate frames to that number; final always appended

#include "triplefill/fill.hpp"
#include "triplefill/image.hpp"
#include "triplefill/pixel.hpp"
#include "triplefill/point.hpp"
#include "triplefill/color_picker.hpp"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <stdexcept>

using namespace triplefill;

namespace {

constexpr int MAX_DIMENSION = 4096;

int    g_last_error = 0;
char   g_last_error_msg[256] = {};

void set_error(int code, const char* msg) {
    g_last_error = code;
    std::snprintf(g_last_error_msg, sizeof(g_last_error_msg), "%s", msg);
}

void clear_error() {
    g_last_error = 0;
    g_last_error_msg[0] = '\0';
}

RGBA rgba_from(double r, double g, double b, double a) {
    return {
        static_cast<uint8_t>(std::clamp(r, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(g, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(b, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(a, 0.0, 255.0))
    };
}

PickerConfig decode_picker(int picker, const double* pp, int pp_len) {
    switch (picker) {
    case 1:
        if (pp_len >= 9) {
            auto sw = static_cast<unsigned>(std::max(1.0, pp[8]));
            return StripePicker{
                rgba_from(pp[0], pp[1], pp[2], pp[3]),
                rgba_from(pp[4], pp[5], pp[6], pp[7]),
                sw
            };
        }
        return StripePicker{RGBA{255, 128, 0}, RGBA{0, 128, 255}, 10};
    case 2:
        if (pp_len >= 7) {
            return QuarterPicker{
                rgba_from(pp[0], pp[1], pp[2], pp[3]),
                static_cast<int>(pp[4]),
                Point{static_cast<int>(pp[5]), static_cast<int>(pp[6])}
            };
        }
        return QuarterPicker{RGBA{255, 0, 0}, 40, {0, 0}};
    case 3:
        if (pp_len >= 9) {
            auto bw = static_cast<unsigned>(std::max(1.0, pp[8]));
            return BorderPicker{
                rgba_from(pp[0], pp[1], pp[2], pp[3]),
                rgba_from(pp[4], pp[5], pp[6], pp[7]),
                bw
            };
        }
        return BorderPicker{RGBA{0, 255, 0}, RGBA{255, 0, 0}, 3};
    default:
        if (pp_len >= 4)
            return SolidPicker{rgba_from(pp[0], pp[1], pp[2], pp[3])};
        return SolidPicker{RGBA{255, 0, 0}};
    }
}

FillConfig build_config(int seed_x, int seed_y, double tolerance,
                        int frame_freq, int algo, int picker,
                        const double* pp, int pp_len, int max_frames) {
    FillConfig cfg;
    cfg.seed       = Point{seed_x, seed_y};
    cfg.tolerance  = std::clamp(tolerance, 0.0, 2.0);
    cfg.frame_freq = std::max(0, frame_freq);
    cfg.algorithm  = (algo == 1) ? Algorithm::DFS : Algorithm::BFS;
    cfg.picker     = decode_picker(picker, pp, pp_len);

    // maxFrames == 0 → unlimited (nullopt); >0 → cap
    if (max_frames > 0)
        cfg.max_frames = static_cast<std::size_t>(max_frames);
    // else: leave as nullopt (unlimited)

    return cfg;
}

} // namespace

extern "C" {

// ---- Error query ----------------------------------------------------------
// Error codes:
//  0  success
//  1  invalid arguments
//  2  allocation failed / OOM
//  3  internal exception

int fill_last_error_code() { return g_last_error; }
const char* fill_last_error_message() { return g_last_error_msg; }

// ---- Stats query ----------------------------------------------------------

int fill_get_filled_pixels(void* handle) {
    if (!handle) return 0;
    return static_cast<int>(static_cast<Animation*>(handle)->stats().filled_pixels);
}

// ---- Legacy single-shot fill (returns final frame only) -------------------
// Return codes: 0=ok, -1=invalid args, -2=empty, -3=OOM, -4=too large, -5=other

int run_fill(
    const uint8_t* rgba_in, int width, int height,
    int seed_x, int seed_y,
    double tolerance, int frame_freq,
    int algo, int picker,
    const double* picker_params, int picker_params_len,
    uint8_t** rgba_out, int* out_size)
{
    clear_error();

    if (!rgba_in || width <= 0 || height <= 0 || !rgba_out || !out_size) {
        set_error(1, "Invalid arguments");
        return -1;
    }
    if (width > MAX_DIMENSION || height > MAX_DIMENSION) {
        set_error(1, "Image too large (max 4096x4096)");
        return -4;
    }

    try {
        Image img(static_cast<unsigned>(width), static_cast<unsigned>(height));
        std::memcpy(img.data(), rgba_in,
                    static_cast<size_t>(width) * height * 4);

        FillConfig cfg = build_config(seed_x, seed_y, tolerance, frame_freq,
                                      algo, picker, picker_params,
                                      picker_params_len, 0);

        Animation anim = flood_fill(img, cfg);
        if (anim.empty()) {
            set_error(1, "Fill produced no result (seed out of bounds?)");
            return -2;
        }

        const Image& final_img = anim.final_frame();
        const size_t bytes = static_cast<size_t>(width) * height * 4;
        auto* buf = static_cast<uint8_t*>(std::malloc(bytes));
        if (!buf) {
            set_error(2, "Out of memory");
            return -3;
        }

        std::memcpy(buf, final_img.data(), bytes);
        *rgba_out = buf;
        *out_size = static_cast<int>(bytes);
        return 0;
    } catch (const std::bad_alloc&) {
        set_error(2, "Out of memory");
        return -3;
    } catch (...) {
        set_error(3, "Unexpected internal error");
        return -5;
    }
}

// ---- Multi-frame fill ----------------------------------------------------

void* fill_create(
    const uint8_t* rgba_in, int width, int height,
    int seed_x, int seed_y,
    double tolerance, int frame_freq,
    int algo, int picker,
    const double* picker_params, int picker_params_len,
    int max_frames)
{
    clear_error();

    if (!rgba_in || width <= 0 || height <= 0) {
        set_error(1, "Invalid arguments (null input or non-positive dimensions)");
        return nullptr;
    }
    if (width > MAX_DIMENSION || height > MAX_DIMENSION) {
        set_error(1, "Image too large (max 4096x4096)");
        return nullptr;
    }

    try {
        Image img(static_cast<unsigned>(width), static_cast<unsigned>(height));
        std::memcpy(img.data(), rgba_in,
                    static_cast<size_t>(width) * height * 4);

        FillConfig cfg = build_config(seed_x, seed_y, tolerance, frame_freq,
                                      algo, picker, picker_params,
                                      picker_params_len, max_frames);

        auto* anim = new Animation(flood_fill(img, cfg));
        if (anim->empty()) {
            delete anim;
            set_error(1, "Fill produced no result (seed out of bounds?)");
            return nullptr;
        }
        return anim;
    } catch (const std::bad_alloc&) {
        set_error(2, "Out of memory — try fewer frames or a smaller image");
        return nullptr;
    } catch (...) {
        set_error(3, "Unexpected internal error");
        return nullptr;
    }
}

int fill_frame_count(void* handle) {
    if (!handle) return 0;
    return static_cast<int>(static_cast<Animation*>(handle)->size());
}

const uint8_t* fill_get_frame(void* handle, int index) {
    if (!handle) return nullptr;
    auto* anim = static_cast<Animation*>(handle);
    if (index < 0 || static_cast<size_t>(index) >= anim->size())
        return nullptr;
    return reinterpret_cast<const uint8_t*>(anim->frame(index).data());
}

void fill_destroy(void* handle) {
    delete static_cast<Animation*>(handle);
}

void free_buffer(void* p) {
    std::free(p);
}

} // extern "C"
