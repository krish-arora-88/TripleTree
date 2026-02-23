#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "triplefill/fill.hpp"
#include "triplefill/image.hpp"

#include <cmath>
#include <filesystem>

using namespace triplefill;
namespace fs = std::filesystem;

static const fs::path FIXTURES = fs::path(TRIPLEFILL_FIXTURES_DIR);

// Helper: create a solid-colour test image
static Image make_solid(unsigned w, unsigned h, RGBA c) {
    return Image(w, h, c);
}

// Helper: pixel-exact comparison
static bool images_match(const Image& a, const Image& b) {
    if (a.width() != b.width() || a.height() != b.height()) return false;
    for (std::size_t i = 0; i < a.pixel_count(); ++i)
        if (!(a.data()[i] == b.data()[i])) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Flood fill on a uniform image should colour every pixel
// ---------------------------------------------------------------------------

TEST_CASE("BFS solid fill on uniform image colours all pixels", "[fill]") {
    auto img = make_solid(20, 15, RGBA{100, 100, 100});
    FillConfig cfg{
        .seed      = {10, 7},
        .tolerance = 0.5,
        .frame_freq = 500,
        .algorithm = Algorithm::BFS,
        .picker    = SolidPicker{RGBA{255, 0, 0}},
    };

    auto anim = flood_fill(img, cfg);
    REQUIRE(!anim.empty());

    const auto& result = anim.final_frame();
    for (unsigned y = 0; y < result.height(); ++y)
        for (unsigned x = 0; x < result.width(); ++x)
            REQUIRE(result.at(x, y) == RGBA{255, 0, 0});
}

TEST_CASE("DFS solid fill on uniform image colours all pixels", "[fill]") {
    auto img = make_solid(20, 15, RGBA{100, 100, 100});
    FillConfig cfg{
        .seed      = {0, 0},
        .tolerance = 0.5,
        .frame_freq = 500,
        .algorithm = Algorithm::DFS,
        .picker    = SolidPicker{RGBA{0, 255, 0}},
    };

    auto anim = flood_fill(img, cfg);
    REQUIRE(!anim.empty());

    const auto& result = anim.final_frame();
    for (unsigned y = 0; y < result.height(); ++y)
        for (unsigned x = 0; x < result.width(); ++x)
            REQUIRE(result.at(x, y) == RGBA{0, 255, 0});
}

// ---------------------------------------------------------------------------
// Stripe fill produces expected pattern
// ---------------------------------------------------------------------------

TEST_CASE("Stripe fill pattern on uniform image", "[fill][stripe]") {
    auto img = make_solid(30, 30, RGBA{200, 200, 200});
    RGBA c1{255, 0, 0};
    RGBA c2{0, 0, 255};
    unsigned sw = 5;

    FillConfig cfg{
        .seed       = {0, 0},
        .tolerance  = 0.5,
        .frame_freq = 0, // no intermediate frames
        .algorithm  = Algorithm::BFS,
        .picker     = StripePicker{c1, c2, sw},
    };

    auto anim = flood_fill(img, cfg);
    const auto& result = anim.final_frame();

    for (unsigned y = 0; y < 30; ++y) {
        for (unsigned x = 0; x < 30; ++x) {
            unsigned idx = (x + y) / sw;
            RGBA expected = (idx % 2 == 0) ? c1 : c2;
            REQUIRE(result.at(x, y) == expected);
        }
    }
}

// ---------------------------------------------------------------------------
// Fill respects tolerance boundary
// ---------------------------------------------------------------------------

TEST_CASE("Fill stops at tolerance boundary", "[fill]") {
    Image img(10, 10, RGBA{100, 100, 100});
    // Paint a vertical barrier of very different colour in column 5
    for (unsigned y = 0; y < 10; ++y)
        img.at(5, y) = RGBA{0, 0, 0};

    FillConfig cfg{
        .seed      = {2, 5},
        .tolerance = 0.05, // very tight
        .frame_freq = 0,
        .algorithm = Algorithm::BFS,
        .picker    = SolidPicker{RGBA{255, 0, 0}},
    };

    auto anim = flood_fill(img, cfg);
    const auto& result = anim.final_frame();

    // Left side should be filled
    REQUIRE(result.at(0, 0) == RGBA{255, 0, 0});
    REQUIRE(result.at(4, 9) == RGBA{255, 0, 0});

    // Barrier and right side should be unchanged
    REQUIRE(result.at(5, 0) == RGBA{0, 0, 0});
    REQUIRE(result.at(6, 0) == RGBA{100, 100, 100});
}

// ---------------------------------------------------------------------------
// Frame frequency
// ---------------------------------------------------------------------------

TEST_CASE("Frame frequency captures correct number of frames", "[fill]") {
    auto img = make_solid(10, 10, RGBA{50, 50, 50}); // 100 pixels total
    FillConfig cfg{
        .seed       = {0, 0},
        .tolerance  = 1.0,
        .frame_freq = 25,
        .algorithm  = Algorithm::BFS,
        .picker     = SolidPicker{RGBA{200, 0, 0}},
    };

    auto anim = flood_fill(img, cfg);
    // 100 pixels, frame every 25 => 4 intermediate frames + 1 final = 5
    REQUIRE(anim.size() == 5);
}

// ---------------------------------------------------------------------------
// Golden PNG round-trip (load -> fill -> save -> reload -> compare)
// ---------------------------------------------------------------------------

TEST_CASE("PNG round-trip: load and re-save is lossless", "[fill][png]") {
    auto originals = FIXTURES / "images-original";
    if (!fs::exists(originals)) {
        WARN("Fixtures directory not found — skipping golden test");
        return;
    }

    auto src = originals / "green-1x1.png";
    if (!fs::exists(src)) {
        WARN("green-1x1.png not found — skipping");
        return;
    }

    auto img = load_png(src);
    REQUIRE(img.width() == 1);
    REQUIRE(img.height() == 1);

    auto tmp = fs::temp_directory_path() / "triplefill_roundtrip.png";
    save_png(tmp, img);
    auto reloaded = load_png(tmp);

    REQUIRE(images_match(img, reloaded));
    fs::remove(tmp);
}

TEST_CASE("Solid fill on fixture image produces deterministic output",
          "[fill][golden]") {
    auto originals = FIXTURES / "images-original";
    auto src = originals / "malachi-60x87.png";
    if (!fs::exists(src)) {
        WARN("malachi-60x87.png not found — skipping golden test");
        return;
    }

    auto img = load_png(src);

    FillConfig cfg{
        .seed       = {30, 43},
        .tolerance  = 0.15,
        .frame_freq = 0,
        .algorithm  = Algorithm::BFS,
        .picker     = SolidPicker{RGBA{255, 0, 128}},
    };

    auto anim1 = flood_fill(img, cfg);
    auto anim2 = flood_fill(img, cfg);

    REQUIRE(images_match(anim1.final_frame(), anim2.final_frame()));
}
