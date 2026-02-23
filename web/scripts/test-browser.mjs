#!/usr/bin/env node
// Test the WASM module by providing the binary directly, bypassing fetch/XHR.
// This validates all C code paths work correctly.

import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { readFileSync } from "node:fs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const wasmDir = resolve(__dirname, "..", "public", "wasm");

// Mock browser worker globals so the Emscripten environment check passes
globalThis.importScripts = () => {};
globalThis.self = globalThis;
if (!globalThis.location) globalThis.location = { href: "file://" + wasmDir + "/" };

const jsText = readFileSync(resolve(wasmDir, "triplefill.js"), "utf8");
const wasmBinary = readFileSync(resolve(wasmDir, "triplefill.wasm"));
const factory = new Function(jsText + "\nreturn TriplefillModule;")();

async function main() {
  console.log("Loading WASM module...");
  const m = await factory({
    wasmBinary: wasmBinary.buffer,
  });
  console.log("Module loaded.\n");

  const W = 200, H = 200;
  const pixelBytes = W * H * 4;

  // Create test image: red bg, blue circle at center
  const inPtr = m._malloc(pixelBytes);
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const off = inPtr + (y * W + x) * 4;
      const dx = x - 100, dy = y - 100;
      const inCircle = dx * dx + dy * dy < 50 * 50;
      m.HEAPU8[off]     = inCircle ? 0 : 255;
      m.HEAPU8[off + 1] = 0;
      m.HEAPU8[off + 2] = inCircle ? 255 : 0;
      m.HEAPU8[off + 3] = 255;
    }
  }

  // Solid green picker params
  const ppPtr = m._malloc(4 * 8);
  m.setValue(ppPtr,      0,   "double");
  m.setValue(ppPtr + 8,  255, "double");
  m.setValue(ppPtr + 16, 0,   "double");
  m.setValue(ppPtr + 24, 255, "double");

  // --- Test 1: run_fill (final only) ---
  console.log("Test 1: run_fill (final only)");
  {
    const outPP = m._malloc(4), outSP = m._malloc(4);
    const rc = m._run_fill(inPtr, W, H, 10, 10, 0.1, 0, 0, 0, ppPtr, 4, outPP, outSP);
    if (rc !== 0) throw new Error("run_fill rc=" + rc);
    const outPtr = m.getValue(outPP, "*");
    const g = m.HEAPU8[outPtr + (10 * W + 10) * 4 + 1];
    if (g !== 255) throw new Error("Seed pixel not green: g=" + g);
    const b = m.HEAPU8[outPtr + (100 * W + 100) * 4 + 2];
    if (b !== 255) throw new Error("Blue area changed: b=" + b);
    m._free_buffer(outPtr); m._free(outPP); m._free(outSP);
    console.log("  PASS\n");
  }

  // --- Test 2: fill_create (with frames) ---
  console.log("Test 2: fill_create (with frames)");
  {
    const handle = m._fill_create(inPtr, W, H, 10, 10, 0.1, 500, 0, 0, ppPtr, 4, 10);
    if (!handle) throw new Error("fill_create returned null");
    const count = m._fill_frame_count(handle);
    if (count <= 0) throw new Error("No frames");
    console.log(`  ${count} frames`);
    for (let i = 0; i < count; i++) {
      const fp = m._fill_get_frame(handle, i);
      if (!fp) throw new Error("Frame " + i + " null");
    }
    // Verify final frame has green at seed
    const lastFp = m._fill_get_frame(handle, count - 1);
    const g = m.HEAPU8[lastFp + (10 * W + 10) * 4 + 1];
    if (g !== 255) throw new Error("Final frame seed not green");
    m._fill_destroy(handle);
    console.log("  PASS\n");
  }

  // --- Test 3: Large image (1000x800) ---
  console.log("Test 3: 1000x800 image, 30 frames");
  {
    const W2 = 1000, H2 = 800, px2 = W2 * H2 * 4;
    const in2 = m._malloc(px2);
    for (let y = 0; y < H2; y++) {
      for (let x = 0; x < W2; x++) {
        const off = in2 + (y * W2 + x) * 4;
        m.HEAPU8[off] = Math.floor((x / W2) * 255);
        m.HEAPU8[off + 1] = Math.floor((y / H2) * 255);
        m.HEAPU8[off + 2] = 128;
        m.HEAPU8[off + 3] = 255;
      }
    }
    const handle = m._fill_create(in2, W2, H2, 100, 100, 0.15, 5000, 0, 0, ppPtr, 4, 30);
    if (!handle) throw new Error("fill_create null on large image");
    const count = m._fill_frame_count(handle);
    console.log(`  ${count} frames`);
    for (let i = 0; i < count; i++) {
      if (!m._fill_get_frame(handle, i)) throw new Error("Frame " + i + " null");
    }
    m._fill_destroy(handle);
    m._free(in2);
    console.log("  PASS\n");
  }

  // --- Test 4: All picker types ---
  console.log("Test 4: All picker types");
  for (const [name, picker, params] of [
    ["solid",   0, [255, 0, 0, 255]],
    ["stripe",  1, [255, 128, 0, 255, 0, 128, 255, 255, 10]],
    ["quarter", 2, [255, 0, 0, 255, 40, 0, 0]],
    ["border",  3, [0, 255, 0, 255, 255, 0, 0, 255, 3]],
  ]) {
    const pp2 = m._malloc(params.length * 8);
    for (let i = 0; i < params.length; i++) m.setValue(pp2 + i * 8, params[i], "double");
    const outPP = m._malloc(4), outSP = m._malloc(4);
    const rc = m._run_fill(inPtr, W, H, 10, 10, 0.1, 0, 0, picker, pp2, params.length, outPP, outSP);
    if (rc !== 0) throw new Error(name + " failed: rc=" + rc);
    m._free_buffer(m.getValue(outPP, "*"));
    m._free(outPP); m._free(outSP); m._free(pp2);
    process.stdout.write(`  ${name} OK  `);
  }
  console.log("\n  PASS\n");

  // --- Test 5: DFS algorithm ---
  console.log("Test 5: DFS algorithm");
  {
    const outPP = m._malloc(4), outSP = m._malloc(4);
    const rc = m._run_fill(inPtr, W, H, 10, 10, 0.1, 0, 1, 0, ppPtr, 4, outPP, outSP);
    if (rc !== 0) throw new Error("DFS run_fill rc=" + rc);
    m._free_buffer(m.getValue(outPP, "*"));
    m._free(outPP); m._free(outSP);
    console.log("  PASS\n");
  }

  m._free(ppPtr);
  m._free(inPtr);
  console.log("All 5 tests passed.");
}

main().catch((e) => { console.error("FATAL:", e); process.exit(1); });
