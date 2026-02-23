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

## Web demo

An interactive in-browser demo powered by WebAssembly (Emscripten). Drag & drop
a PNG, click a seed point, choose an algorithm and picker, and watch the fill
happen — all computed off-thread in a Web Worker.

### Prerequisites

- **Emscripten SDK** — install from
  <https://emscripten.org/docs/getting_started/downloads.html> and source
  `emsdk_env.sh` so that `emcmake` / `emmake` are on PATH.
- **Node.js** ≥ 18

### Quick start

```bash
# 1. Build the WASM module
cd web
npm install
npm run build:wasm        # configures + compiles triplefill → WASM

# 2. Start the dev server
npm run dev               # opens http://localhost:5173
```

A single `npm run build` produces a fully static `web/dist/` directory that
can be deployed to GitHub Pages or any static host.

### How it works

```
┌──────────────────────────────────────────────────────────┐
│  Browser (main thread)                                   │
│  ┌──────────┐  RGBA buffer   ┌──────────────────────┐   │
│  │  Canvas   │ ──────────▶  │  Web Worker            │   │
│  │  (input)  │              │  loads triplefill.wasm  │   │
│  └──────────┘  ◀──────────  │  calls run_fill()      │   │
│  ┌──────────┐  RGBA frames  └──────────────────────┘   │
│  │  Canvas   │              Transferable ArrayBuffers    │
│  │  (output) │              (zero-copy)                  │
│  └──────────┘                                            │
└──────────────────────────────────────────────────────────┘
```

The WASM module exposes a minimal C ABI (`run_fill`, `run_fill_with_frames`,
`free_buffer`). No internal C++ classes are exposed. The JS worker marshals
RGBA buffers in and out and uses `Transferable` objects to avoid copies.

### Known limitations

- Very large images (> 4000 × 4000) may be slow or exhaust WASM memory.
  The module caps at 1 GB.
- Frame capture with many frames increases memory proportionally;
  use `max_frames` to cap.
- The WASM module is built with `ENVIRONMENT='worker'` — it cannot be loaded
  on the main thread.

## Project layout

```
include/triplefill/    Public headers (library API)
src/                   Library implementation
apps/cli/              Command-line frontend
apps/wasm/             WebAssembly bindings (Emscripten)
web/                   Vite + TypeScript web frontend
  src/                 Application source
    worker/            Web Worker (loads WASM, runs fills)
  public/wasm/         WASM build artifacts (generated)
  scripts/             Build automation
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
