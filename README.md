# triplefill

A modern C++20 flood-fill animation library and CLI tool.

Given a PNG image, a seed pixel, and a colour-tolerance threshold,
**triplefill** performs BFS or DFS flood fill, captures animation frames at
configurable intervals, and writes the result as a PNG or animated GIF — all
with zero external runtime dependencies.

## Features

- **BFS / DFS flood fill** with configurable neighbour ordering (N-E-S-W).
- **HSL colour-distance tolerance** for natural-looking fill boundaries.
- **Pluggable colour pickers**: solid, diagonal stripe, quadrant luminance,
  and border-aware fill.
- **Animation capture**: frame snapshots every *k* pixels filled.
- **GIF export**: built-in LZW encoder with median-cut quantisation — no
  ImageMagick, no `system()` calls.
- **Clean library + CLI** separation; easy to embed in other projects.
- **Cross-platform**: builds and tests on Linux, macOS, and Windows via CMake.

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## CLI usage

```bash
# Solid red fill, BFS, tolerance 0.15
./build/apps/cli/triplefill \
    --input photo.png \
    --output filled.png \
    --seed 100,200 \
    --tolerance 0.15 \
    --algo bfs \
    --picker solid --color 255,0,0,255

# Diagonal stripe fill with animated GIF output
./build/apps/cli/triplefill \
    --input photo.png \
    --output animation.gif \
    --seed 50,50 \
    --tolerance 0.2 \
    --frame-freq 500 \
    --picker stripe --color1 255,128,0,255 --color2 0,128,255,255 \
    --stripe-width 8

# Border-aware fill
./build/apps/cli/triplefill \
    --input photo.png \
    --output bordered.png \
    --seed 0,0 \
    --tolerance 0.3 \
    --picker border --color 0,255,0,255 \
    --border-color 255,0,0,255 --border-width 4
```

Run `triplefill --help` for the full option list.

## Project layout

```
include/triplefill/    Public headers (library API)
src/                   Library implementation
apps/cli/              Command-line frontend
tests/                 Catch2 unit + golden-image tests
third_party/           Vendored dependencies (lodepng, Catch2, gif encoder)
cmake/                 CMake helpers (sanitisers, toolchains)
legacy/                Original class-project code (not built)
```

## Algorithms

### Flood fill

The engine uses a visited bitmap and a work structure:

| Algorithm | Structure          | Behaviour        |
|-----------|--------------------|------------------|
| BFS       | `std::deque`       | FIFO — level-order expansion |
| DFS       | `std::vector`      | LIFO — depth-first expansion |

Neighbour push order is always **North → East → South → West**.
A pixel is marked *visited* on push and *coloured* on pop.

### Tolerance

Colour distance is computed in HSL space:

```
d = sqrt(Δh² + Δs² + Δl²)
```

where Δh uses the shortest arc on the hue circle (normalised to [0, 1]).

## Performance notes

- The hot loop avoids heap allocation: `std::deque` and `std::vector` are
  reused; the visited bitmap is a flat `vector<uint8_t>`.
- Colour pickers use `std::variant` dispatch (no virtual-call overhead) via
  `std::visit`; trivial pickers (solid, stripe) are inlined.
- The GIF encoder writes directly to `FILE*` with sub-block buffering;
  no temporary in-memory copy of the entire encoded stream.
- For very large images, use `--frame-freq 0` to skip intermediate frame
  capture and reduce memory from O(frames × pixels) to O(pixels).

## Building with sanitisers

```bash
cmake --preset sanitize
cmake --build build-sanitize
ctest --test-dir build-sanitize
```

## License

MIT — see [LICENSE](LICENSE).
