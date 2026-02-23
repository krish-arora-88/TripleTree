// Core application logic for the triplefill web demo.

const $ = <T extends HTMLElement>(sel: string) =>
  document.querySelector<T>(sel)!;

// ---- State -----------------------------------------------------------------

interface AppState {
  imageData: ImageData | null;
  seedX: number;
  seedY: number;
  seedSet: boolean;
  frames: ArrayBuffer[];
  currentFrame: number;
  running: boolean;
}

const state: AppState = {
  imageData: null,
  seedX: 0,
  seedY: 0,
  seedSet: false,
  frames: [],
  currentFrame: 0,
  running: false,
};

// ---- DOM refs --------------------------------------------------------------

const dropZone = $<HTMLDivElement>("#drop-zone");
const fileInput = $<HTMLInputElement>("#file-input");
const inputWrap = $<HTMLDivElement>("#input-canvas-wrap");
const inputCanvas = $<HTMLCanvasElement>("#input-canvas");
const seedMarker = $<HTMLDivElement>("#seed-marker");
const algoSelect = $<HTMLSelectElement>("#algo-select");
const tolSlider = $<HTMLInputElement>("#tolerance-slider");
const tolValue = $<HTMLSpanElement>("#tolerance-value");
const pickerSelect = $<HTMLSelectElement>("#picker-select");
const pickerParams = $<HTMLDivElement>("#picker-params");
const frameFreqInput = $<HTMLInputElement>("#frame-freq-input");
const maxFramesInput = $<HTMLInputElement>("#max-frames-input");
const runBtn = $<HTMLButtonElement>("#run-btn");
const warningBanner = $<HTMLDivElement>("#warning-banner");
const statusEl = $<HTMLDivElement>("#status");
const outputWrap = $<HTMLDivElement>("#output-canvas-wrap");
const outputCanvas = $<HTMLCanvasElement>("#output-canvas");
const outputPlaceholder = $<HTMLDivElement>("#output-placeholder");
const frameScrubber = $<HTMLDivElement>("#frame-scrubber");
const frameSlider = $<HTMLInputElement>("#frame-slider");
const frameIndex = $<HTMLSpanElement>("#frame-index");
const frameTotal = $<HTMLSpanElement>("#frame-total");
const exportBtns = $<HTMLDivElement>("#export-buttons");
const downloadPng = $<HTMLButtonElement>("#download-png");
const uploadAnother = $<HTMLButtonElement>("#upload-another");
const seedSizeSlider = $<HTMLInputElement>("#seed-size-slider");
const seedSizeValue = $<HTMLSpanElement>("#seed-size-value");

// ---- Worker ----------------------------------------------------------------

let worker: Worker | null = null;

function getWorker(): Worker {
  if (!worker) {
    // Classic worker from public/ — uses importScripts to load Emscripten
    // (most reliable cross-browser approach for WASM workers)
    worker = new Worker(new URL("/fillWorker.js", import.meta.url));
  }
  return worker;
}

// ---- Image loading ---------------------------------------------------------

const MAX_DIM = 4096;

async function loadImage(file: File): Promise<void> {
  const bitmap = await createImageBitmap(file);
  let w = bitmap.width;
  let h = bitmap.height;

  // Downscale if either dimension exceeds the WASM limit
  if (w > MAX_DIM || h > MAX_DIM) {
    const scale = Math.min(MAX_DIM / w, MAX_DIM / h);
    w = Math.floor(w * scale);
    h = Math.floor(h * scale);
  }

  inputCanvas.width = w;
  inputCanvas.height = h;
  const ctx = inputCanvas.getContext("2d")!;
  ctx.drawImage(bitmap, 0, 0, w, h);
  bitmap.close();

  state.imageData = ctx.getImageData(0, 0, w, h);
  state.frames = [];

  // Auto-seed to image center so the Run button is immediately usable
  state.seedX = Math.floor(w / 2);
  state.seedY = Math.floor(h / 2);
  state.seedSet = true;

  dropZone.hidden = true;
  inputWrap.hidden = false;
  outputWrap.hidden = true;
  outputPlaceholder.hidden = false;
  frameScrubber.hidden = true;
  exportBtns.hidden = true;

  uploadAnother.hidden = false;

  requestAnimationFrame(() => {
    const rect = inputCanvas.getBoundingClientRect();
    applySeedSize();
    seedMarker.style.left = `${rect.width / 2}px`;
    seedMarker.style.top = `${rect.height / 2}px`;
    seedMarker.hidden = false;
  });

  updateRunButton();
}

// ---- Seed picking ----------------------------------------------------------

function applySeedSize(): void {
  seedMarker.style.setProperty("--marker-size", `${seedSizeSlider.value}px`);
}

function onCanvasClick(e: MouseEvent): void {
  if (!state.imageData) return;

  const rect = inputCanvas.getBoundingClientRect();
  const scaleX = inputCanvas.width / rect.width;
  const scaleY = inputCanvas.height / rect.height;

  state.seedX = Math.floor((e.clientX - rect.left) * scaleX);
  state.seedY = Math.floor((e.clientY - rect.top) * scaleY);
  state.seedX = Math.max(0, Math.min(state.seedX, inputCanvas.width - 1));
  state.seedY = Math.max(0, Math.min(state.seedY, inputCanvas.height - 1));
  state.seedSet = true;

  applySeedSize();
  const markerX = (state.seedX / inputCanvas.width) * rect.width;
  const markerY = (state.seedY / inputCanvas.height) * rect.height;
  seedMarker.style.left = `${markerX}px`;
  seedMarker.style.top = `${markerY}px`;
  seedMarker.hidden = false;

  updateRunButton();
}

// ---- Picker params UI ------------------------------------------------------

function renderPickerParams(): void {
  const picker = parseInt(pickerSelect.value, 10);
  let html = "";

  switch (picker) {
    case 0: // solid
      html = `
        <div class="control-group">
          <div class="color-row">
            <label>Fill color</label>
            <input type="color" id="pp-color" value="#ff0000" />
          </div>
        </div>`;
      break;

    case 1: // stripe
      html = `
        <div class="control-group">
          <div class="color-row">
            <label>Color 1</label>
            <input type="color" id="pp-color1" value="#ff8000" />
          </div>
          <div class="color-row">
            <label>Color 2</label>
            <input type="color" id="pp-color2" value="#0080ff" />
          </div>
          <label for="pp-stripe-width">Stripe width</label>
          <input type="number" id="pp-stripe-width" min="1" max="100" value="10" />
        </div>`;
      break;

    case 2: // quarter
      html = `
        <div class="control-group">
          <div class="color-row">
            <label>Base color</label>
            <input type="color" id="pp-color" value="#ff0000" />
          </div>
          <label for="pp-bright">Brightness delta</label>
          <input type="number" id="pp-bright" min="-100" max="100" value="40" />
        </div>`;
      break;

    case 3: // border
      html = `
        <div class="control-group">
          <div class="color-row">
            <label>Fill color</label>
            <input type="color" id="pp-fill-color" value="#00ff00" />
          </div>
          <div class="color-row">
            <label>Border color</label>
            <input type="color" id="pp-border-color" value="#ff0000" />
          </div>
          <label for="pp-border-width">Border width</label>
          <input type="number" id="pp-border-width" min="1" max="20" value="3" />
        </div>`;
      break;
  }

  pickerParams.innerHTML = html;
}

function hexToRGBA(hex: string): [number, number, number, number] {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return [r, g, b, 255];
}

function getPickerParamsArray(): number[] {
  const picker = parseInt(pickerSelect.value, 10);

  switch (picker) {
    case 0: {
      const c = hexToRGBA(($<HTMLInputElement>("#pp-color")?.value) ?? "#ff0000");
      return [...c];
    }
    case 1: {
      const c1 = hexToRGBA($<HTMLInputElement>("#pp-color1")?.value ?? "#ff8000");
      const c2 = hexToRGBA($<HTMLInputElement>("#pp-color2")?.value ?? "#0080ff");
      const sw = parseInt($<HTMLInputElement>("#pp-stripe-width")?.value ?? "10", 10);
      return [...c1, ...c2, sw];
    }
    case 2: {
      const c = hexToRGBA($<HTMLInputElement>("#pp-color")?.value ?? "#ff0000");
      const bright = parseInt($<HTMLInputElement>("#pp-bright")?.value ?? "40", 10);
      return [...c, bright, 0, 0];
    }
    case 3: {
      const fc = hexToRGBA($<HTMLInputElement>("#pp-fill-color")?.value ?? "#00ff00");
      const bc = hexToRGBA($<HTMLInputElement>("#pp-border-color")?.value ?? "#ff0000");
      const bw = parseInt($<HTMLInputElement>("#pp-border-width")?.value ?? "3", 10);
      return [...fc, ...bc, bw];
    }
    default:
      return [255, 0, 0, 255];
  }
}

// ---- Run fill --------------------------------------------------------------

function updateRunButton(): void {
  runBtn.disabled = !state.imageData || !state.seedSet || state.running;
}

function setStatus(text: string, cls: "running" | "done" | "error"): void {
  statusEl.hidden = false;
  statusEl.className = `status ${cls}`;
  statusEl.innerHTML = text;
}

function showWarning(text: string | undefined): void {
  if (text) {
    warningBanner.hidden = false;
    warningBanner.textContent = text;
  } else {
    warningBanner.hidden = true;
    warningBanner.textContent = "";
  }
}

function validateParams(): string | null {
  const tol = parseFloat(tolSlider.value);
  if (isNaN(tol) || tol < 0 || tol > 2) return "Tolerance must be between 0 and 2.";

  const freq = parseInt(frameFreqInput.value, 10);
  if (isNaN(freq) || freq < 0) return "Frame frequency must be >= 0.";

  const maxF = parseInt(maxFramesInput.value, 10);
  if (isNaN(maxF) || maxF < 0) return "Max frames must be >= 0.";

  return null;
}

function runFill(): void {
  if (!state.imageData || !state.seedSet || state.running) return;

  const validationError = validateParams();
  if (validationError) {
    setStatus(`Error: ${validationError}`, "error");
    return;
  }

  state.running = true;
  updateRunButton();
  showWarning(undefined);
  setStatus('<span class="spinner"></span> Running...', "running");

  const rgba = state.imageData.data.buffer.slice(0);
  const w = getWorker();

  const msg = {
    type: "fill" as const,
    rgba,
    width: state.imageData.width,
    height: state.imageData.height,
    seedX: state.seedX,
    seedY: state.seedY,
    tolerance: parseFloat(tolSlider.value),
    frameFreq: Math.max(0, parseInt(frameFreqInput.value, 10) || 0),
    algo: parseInt(algoSelect.value, 10),
    picker: parseInt(pickerSelect.value, 10),
    pickerParams: getPickerParamsArray(),
    maxFrames: Math.max(0, parseInt(maxFramesInput.value, 10) || 0),
  };

  w.onmessage = (e: MessageEvent) => {
    state.running = false;
    updateRunButton();

    if (e.data.type === "error") {
      setStatus(`Error: ${e.data.message}`, "error");
      return;
    }

    const { frames, width, height, frameCount, timeMs, warning } = e.data;
    state.frames = frames;
    state.currentFrame = frameCount - 1;

    showWarning(warning);

    let statusText = `Done in ${timeMs} ms &mdash; ${frameCount} frame${frameCount > 1 ? "s" : ""}`;
    if (warning && frameCount <= 1) {
      statusText += " (frame capture limited)";
    }
    setStatus(statusText, warning ? "done" : "done");
    showFrame(width, height, frameCount - 1);

    if (frameCount > 1) {
      frameScrubber.hidden = false;
      frameSlider.max = String(frameCount - 1);
      frameSlider.value = String(frameCount - 1);
      frameTotal.textContent = String(frameCount);
      frameIndex.textContent = String(frameCount);
    } else {
      frameScrubber.hidden = true;
    }

    outputPlaceholder.hidden = true;
    outputWrap.hidden = false;
    exportBtns.hidden = false;
  };

  w.postMessage(msg, [rgba]);
}

function showFrame(width: number, height: number, index: number): void {
  const buf = state.frames[index];
  if (!buf) return;

  outputCanvas.width = width;
  outputCanvas.height = height;
  const ctx = outputCanvas.getContext("2d")!;
  const imgData = new ImageData(new Uint8ClampedArray(buf), width, height);
  ctx.putImageData(imgData, 0, 0);
  state.currentFrame = index;
}

// ---- Export ----------------------------------------------------------------

function downloadCanvasPng(): void {
  if (state.frames.length === 0) return;

  outputCanvas.toBlob((blob) => {
    if (!blob) return;
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = "triplefill-output.png";
    a.click();
    URL.revokeObjectURL(url);
  }, "image/png");
}

// ---- Events ----------------------------------------------------------------

export function init(): void {
  renderPickerParams();

  // Drag & drop
  dropZone.addEventListener("dragover", (e) => {
    e.preventDefault();
    dropZone.classList.add("drag-over");
  });
  dropZone.addEventListener("dragleave", () => {
    dropZone.classList.remove("drag-over");
  });
  dropZone.addEventListener("drop", (e) => {
    e.preventDefault();
    dropZone.classList.remove("drag-over");
    const file = e.dataTransfer?.files[0];
    if (file) loadImage(file);
  });

  // File input
  fileInput.addEventListener("change", () => {
    const file = fileInput.files?.[0];
    if (file) loadImage(file);
  });

  // Allow clicking the drop zone area (but not the label/button) to open file picker
  dropZone.addEventListener("click", (e) => {
    const target = e.target as HTMLElement;
    if (target.closest("label") || target.tagName === "INPUT") return;
    fileInput.click();
  });

  // Upload another image
  uploadAnother.addEventListener("click", () => {
    fileInput.value = "";
    fileInput.click();
  });

  // Seed size slider
  seedSizeSlider.addEventListener("input", () => {
    seedSizeValue.textContent = seedSizeSlider.value;
    applySeedSize();
  });

  // Canvas seed picking
  inputCanvas.addEventListener("click", onCanvasClick);

  // Tolerance slider
  tolSlider.addEventListener("input", () => {
    tolValue.textContent = parseFloat(tolSlider.value).toFixed(2);
  });

  // Picker select
  pickerSelect.addEventListener("change", renderPickerParams);

  // Run
  runBtn.addEventListener("click", runFill);

  // Frame scrubbing
  frameSlider.addEventListener("input", () => {
    const idx = parseInt(frameSlider.value, 10);
    frameIndex.textContent = String(idx + 1);
    if (state.imageData) {
      showFrame(state.imageData.width, state.imageData.height, idx);
    }
  });

  // Download
  downloadPng.addEventListener("click", downloadCanvasPng);
}
