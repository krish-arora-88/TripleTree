#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "triplefill/color_picker.hpp"
#include "triplefill/pickers/border.hpp"
#include "triplefill/pickers/quarter.hpp"
#include "triplefill/pickers/solid.hpp"
#include "triplefill/pickers/stripe.hpp"
#include "triplefill/tolerance.hpp"

using namespace triplefill;

// ---------------------------------------------------------------------------
// SolidPicker
// ---------------------------------------------------------------------------

TEST_CASE("SolidPicker returns constant colour", "[pickers][solid]") {
    SolidPicker sp{RGBA{42, 128, 200, 255}};
    REQUIRE(pick_solid(sp, {0, 0}, RGBA{}) == RGBA{42, 128, 200, 255});
    REQUIRE(pick_solid(sp, {999, 123}, RGBA{1, 2, 3}) == RGBA{42, 128, 200, 255});
}

// ---------------------------------------------------------------------------
// StripePicker
// ---------------------------------------------------------------------------

TEST_CASE("StripePicker diagonal stripes", "[pickers][stripe]") {
    StripePicker sp{RGBA{255, 0, 0}, RGBA{0, 0, 255}, 10};

    SECTION("origin is colour1") {
        REQUIRE(pick_stripe(sp, {0, 0}, RGBA{}) == RGBA{255, 0, 0});
    }
    SECTION("within first stripe -> colour1") {
        REQUIRE(pick_stripe(sp, {3, 4}, RGBA{}) == RGBA{255, 0, 0});
    }
    SECTION("second stripe -> colour2") {
        REQUIRE(pick_stripe(sp, {5, 5}, RGBA{}) == RGBA{0, 0, 255});
    }
    SECTION("third stripe -> colour1 again") {
        REQUIRE(pick_stripe(sp, {10, 10}, RGBA{}) == RGBA{255, 0, 0});
    }
    SECTION("parity rule: (x+y)/w even -> c1") {
        for (int x = 0; x < 40; ++x) {
            for (int y = 0; y < 40; ++y) {
                unsigned idx = static_cast<unsigned>(x + y) / 10;
                RGBA expected = (idx % 2 == 0) ? RGBA{255, 0, 0}
                                               : RGBA{0, 0, 255};
                REQUIRE(pick_stripe(sp, {x, y}, RGBA{}) == expected);
            }
        }
    }
}

TEST_CASE("StripePicker width=1 gives checkerboard diagonals",
          "[pickers][stripe]") {
    StripePicker sp{RGBA{1, 1, 1}, RGBA{2, 2, 2}, 1};
    REQUIRE(pick_stripe(sp, {0, 0}, RGBA{}) == RGBA{1, 1, 1});
    REQUIRE(pick_stripe(sp, {1, 0}, RGBA{}) == RGBA{2, 2, 2});
    REQUIRE(pick_stripe(sp, {0, 1}, RGBA{}) == RGBA{2, 2, 2});
    REQUIRE(pick_stripe(sp, {1, 1}, RGBA{}) == RGBA{1, 1, 1});
}

// ---------------------------------------------------------------------------
// QuarterPicker
// ---------------------------------------------------------------------------

TEST_CASE("QuarterPicker varies by quadrant", "[pickers][quarter]") {
    RGBA base{128, 128, 128, 255};
    QuarterPicker qp{base, 40, {50, 50}};

    RGBA tl = pick_quarter(qp, {10, 10}, RGBA{});
    RGBA tr = pick_quarter(qp, {60, 10}, RGBA{});
    RGBA bl = pick_quarter(qp, {10, 60}, RGBA{});
    RGBA br = pick_quarter(qp, {60, 60}, RGBA{});

    SECTION("top-left is unmodified base") {
        REQUIRE(tl == base);
    }
    SECTION("quadrants differ from each other") {
        REQUIRE(tl != tr);
        REQUIRE(tr != bl);
        REQUIRE(bl != br);
    }
}

// ---------------------------------------------------------------------------
// BorderPicker
// ---------------------------------------------------------------------------

TEST_CASE("BorderPicker returns border colour near unfilled neighbours",
          "[pickers][border]") {
    const unsigned W = 10, H = 10;
    std::vector<uint8_t> visited(W * H, 0);
    // Fill a 5x5 block in the centre
    for (unsigned y = 3; y < 8; ++y)
        for (unsigned x = 3; x < 8; ++x)
            visited[y * W + x] = 1;

    BorderPicker bp{RGBA{0, 255, 0}, RGBA{255, 0, 0}, 1};

    SECTION("interior pixel returns fill colour") {
        REQUIRE(pick_border(bp, visited, W, H, {5, 5}, RGBA{}) ==
                RGBA{0, 255, 0});
    }
    SECTION("edge pixel returns border colour") {
        REQUIRE(pick_border(bp, visited, W, H, {3, 5}, RGBA{}) ==
                RGBA{255, 0, 0});
    }
}

// ---------------------------------------------------------------------------
// Tolerance
// ---------------------------------------------------------------------------

TEST_CASE("color_distance identical colours is zero", "[tolerance]") {
    REQUIRE(color_distance(RGBA{100, 150, 200}, RGBA{100, 150, 200}) ==
            Approx(0.0).margin(1e-6));
}

TEST_CASE("color_distance black vs white is large", "[tolerance]") {
    double d = color_distance(RGBA{0, 0, 0}, RGBA{255, 255, 255});
    REQUIRE(d > 0.5);
}

TEST_CASE("HSL round-trip preserves colour", "[tolerance]") {
    RGBA orig{180, 60, 220, 200};
    HSL hsl = rgb_to_hsl(orig);
    RGBA back = hsl_to_rgb(hsl, orig.a);
    REQUIRE(std::abs(static_cast<int>(orig.r) - back.r) <= 1);
    REQUIRE(std::abs(static_cast<int>(orig.g) - back.g) <= 1);
    REQUIRE(std::abs(static_cast<int>(orig.b) - back.b) <= 1);
    REQUIRE(back.a == orig.a);
}
