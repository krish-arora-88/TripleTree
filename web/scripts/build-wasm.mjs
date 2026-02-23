#!/usr/bin/env node
// Build the triplefill WASM module and copy artifacts into web/public/wasm/.
//
// Usage:
//   node scripts/build-wasm.mjs           # uses default build-wasm dir
//   node scripts/build-wasm.mjs --clean   # rm build dir first
//
// Prerequisites: Emscripten SDK installed and `emsdk_env.sh` sourced so that
// `emcmake` and `emmake` are on PATH.

import { execSync } from "node:child_process";
import { existsSync, mkdirSync, cpSync, rmSync } from "node:fs";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, "..", "..");
const BUILD_DIR = resolve(ROOT, "build-wasm");
const OUT_DIR = resolve(__dirname, "..", "public", "wasm");

const clean = process.argv.includes("--clean");

function run(cmd, cwd = ROOT) {
  console.log(`> ${cmd}`);
  execSync(cmd, { cwd, stdio: "inherit" });
}

// Verify emcmake is available (emcmake has no --version; `which` suffices)
try {
  execSync("which emcmake", { stdio: "ignore" });
} catch {
  console.error(
    "Error: emcmake not found. Install Emscripten and source emsdk_env.sh first.\n" +
    "  https://emscripten.org/docs/getting_started/downloads.html"
  );
  process.exit(1);
}

if (clean && existsSync(BUILD_DIR)) {
  console.log("Cleaning build directory...");
  rmSync(BUILD_DIR, { recursive: true });
}

mkdirSync(BUILD_DIR, { recursive: true });
mkdirSync(OUT_DIR, { recursive: true });

// Configure
run(
  `emcmake cmake -S "${ROOT}" -B "${BUILD_DIR}" ` +
  `-DCMAKE_BUILD_TYPE=Release ` +
  `-DTRIPLEFILL_BUILD_CLI=OFF ` +
  `-DTRIPLEFILL_BUILD_TESTS=OFF ` +
  `-DTRIPLEFILL_BUILD_WASM=ON`
);

// Build
run(`emmake cmake --build "${BUILD_DIR}" --parallel --target triplefill_wasm`);

// Copy artifacts
const jsFile = resolve(BUILD_DIR, "apps", "wasm", "triplefill.js");
const wasmFile = resolve(BUILD_DIR, "apps", "wasm", "triplefill.wasm");

if (!existsSync(jsFile) || !existsSync(wasmFile)) {
  console.error("Build succeeded but output files not found.");
  console.error(`  Expected: ${jsFile}`);
  console.error(`  Expected: ${wasmFile}`);
  process.exit(1);
}

cpSync(jsFile, resolve(OUT_DIR, "triplefill.js"));
cpSync(wasmFile, resolve(OUT_DIR, "triplefill.wasm"));

console.log(`\nWASM build complete. Artifacts copied to ${OUT_DIR}`);
