#!/usr/bin/env node
// Test the WASM module directly in Node.js to catch errors with full diagnostics.

import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { readFileSync } from "node:fs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const wasmDir = resolve(__dirname, "..", "public", "wasm");

// Load the Emscripten module (it uses module.exports with CJS,
// so we eval it to extract the factory function)
const jsPath = resolve(wasmDir, "triplefill.js");
const jsText = readFileSync(jsPath, "utf8");
const mod = { exports: {} };
new Function("module", "exports", "require", "__filename", "__dirname", jsText)(
  mod, mod.exports, (await import("node:module")).createRequire(import.meta.url),
  jsPath, wasmDir
);
const factory = mod.exports.default || mod.exports;

async function main() {
  console.log("Loading WASM module...");
  const m = await factory();
  console.log("Module loaded. Testing APIs...\n");

  // Create a simple 100x100 test image: red square with a blue quadrant
  const W = 100, H = 100;
  const pixels = W * H * 4;
  const inPtr = m._malloc(pixels);
  const heap = m.HEAPU8;

  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const off = inPtr + (y * W + x) * 4;
      if (x < 50 && y < 50) {
        heap[off] = 255; heap[off+1] = 0; heap[off+2] = 0; heap[off+3] = 255; // red
      } else {
        heap[off] = 0; heap[off+1] = 0; heap[off+2] = 255; heap[off+3] = 255; // blue
      }
    }
  }

  // Picker params: solid green [0, 255, 0, 255]
  const ppPtr = m._malloc(4 * 8);
  m.setValue(ppPtr + 0, 0, "double");
  m.setValue(ppPtr + 8, 255, "double");
  m.setValue(ppPtr + 16, 0, "double");
  m.setValue(ppPtr + 24, 255, "double");

  // --- Test 1: run_fill (final frame only) ---
  console.log("=== Test 1: run_fill (final frame only) ===");
  {
    const outPtrPtr = m._malloc(4);
    const outSizePtr = m._malloc(4);

    const rc = m._run_fill(
      inPtr, W, H,
      25, 25,       // seed in the red quadrant
      0.1, 0,       // tolerance, frame_freq=0
      0,            // BFS
      0,            // solid picker
      ppPtr, 4,
      outPtrPtr, outSizePtr
    );

    console.log(`  rc = ${rc}`);
    if (rc === 0) {
      const outPtr = m.getValue(outPtrPtr, "*");
      const outSize = m.getValue(outSizePtr, "i32");
      console.log(`  outSize = ${outSize} (expected ${pixels})`);

      // Check that the seed area is now green
      const g = m.HEAPU8[outPtr + (25 * W + 25) * 4 + 1];
      console.log(`  pixel(25,25).g = ${g} (expected 255)`);

      // Blue area should be unchanged
      const b = m.HEAPU8[outPtr + (75 * W + 75) * 4 + 2];
      console.log(`  pixel(75,75).b = ${b} (expected 255)`);

      m._free_buffer(outPtr);
    }
    m._free(outPtrPtr);
    m._free(outSizePtr);
  }

  // --- Test 2: fill_create + iterator (with frames) ---
  console.log("\n=== Test 2: fill_create + iterator (with frames) ===");
  {
    const handle = m._fill_create(
      inPtr, W, H,
      25, 25,       // seed in the red quadrant
      0.1, 500,     // tolerance, frame_freq=500
      0,            // BFS
      0,            // solid picker
      ppPtr, 4,
      10            // max 10 frames
    );

    console.log(`  handle = ${handle}`);
    if (handle) {
      const count = m._fill_frame_count(handle);
      console.log(`  frame_count = ${count}`);

      for (let i = 0; i < count; i++) {
        const fptr = m._fill_get_frame(handle, i);
        console.log(`  frame[${i}] ptr = ${fptr}, valid = ${fptr !== 0}`);
      }

      // Check final frame (last one)
      const lastPtr = m._fill_get_frame(handle, count - 1);
      if (lastPtr) {
        const g = m.HEAPU8[lastPtr + (25 * W + 25) * 4 + 1];
        console.log(`  final pixel(25,25).g = ${g} (expected 255)`);
      }

      m._fill_destroy(handle);
      console.log("  Destroyed handle OK");
    }
  }

  // --- Test 3: larger image (simulate real photo) ---
  console.log("\n=== Test 3: 1000x800 image ===");
  {
    const W2 = 1000, H2 = 800;
    const px2 = W2 * H2 * 4;
    const inPtr2 = m._malloc(px2);
    console.log(`  Allocated ${px2} bytes at ${inPtr2}`);

    // Fill with a gradient
    for (let y = 0; y < H2; y++) {
      for (let x = 0; x < W2; x++) {
        const off = inPtr2 + (y * W2 + x) * 4;
        const v = Math.floor((x / W2) * 255);
        m.HEAPU8[off] = v;
        m.HEAPU8[off+1] = v;
        m.HEAPU8[off+2] = v;
        m.HEAPU8[off+3] = 255;
      }
    }

    // Test run_fill (final only)
    const outPtrPtr = m._malloc(4);
    const outSizePtr = m._malloc(4);
    const rc = m._run_fill(
      inPtr2, W2, H2,
      100, 100,     // seed near left (dark area)
      0.15, 0,      // tolerance, no frames
      0, 0,         // BFS, solid
      ppPtr, 4,
      outPtrPtr, outSizePtr
    );
    console.log(`  run_fill rc = ${rc}`);
    if (rc === 0) {
      const outPtr = m.getValue(outPtrPtr, "*");
      console.log(`  Output pointer = ${outPtr}, size = ${m.getValue(outSizePtr, "i32")}`);
      m._free_buffer(outPtr);
    }
    m._free(outPtrPtr);
    m._free(outSizePtr);

    // Test fill_create with frames on larger image
    console.log("  Testing fill_create with frames on 1000x800...");
    const handle = m._fill_create(
      inPtr2, W2, H2,
      100, 100,
      0.15, 5000,   // frame every 5000 pixels
      0, 0,
      ppPtr, 4,
      30            // max 30 frames
    );
    console.log(`  handle = ${handle}`);
    if (handle) {
      const count = m._fill_frame_count(handle);
      console.log(`  frame_count = ${count}`);
      // Pull each frame
      for (let i = 0; i < count; i++) {
        const fptr = m._fill_get_frame(handle, i);
        if (!fptr) { console.log(`  ERROR: frame ${i} is null!`); break; }
      }
      console.log(`  All ${count} frame pointers valid`);
      m._fill_destroy(handle);
    }

    m._free(inPtr2);
  }

  m._free(ppPtr);
  m._free(inPtr);

  console.log("\n=== All tests passed ===");
}

main().catch(e => {
  console.error("FATAL:", e);
  process.exit(1);
});
