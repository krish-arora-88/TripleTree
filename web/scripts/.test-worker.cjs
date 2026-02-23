
const { parentPort, workerData } = require("worker_threads");
const path = require("path");

// Load the WASM module
const loaded = require(path.join(workerData.wasmDir, "triplefill.js"));
parentPort.postMessage("loaded type: " + typeof loaded + ", keys: " + Object.keys(loaded || {}).join(","));
const factory = typeof loaded === 'function' ? loaded : (loaded.default || loaded.TriplefillModule || loaded);

async function run() {
  const m = await factory({
    locateFile: function(p) {
      return path.join(workerData.wasmDir, p);
    }
  });

  const msg = workerData.msg;
  const pixelBytes = msg.width * msg.height * 4;

  // Copy input RGBA into WASM heap
  const inPtr = m._malloc(pixelBytes);
  m.HEAPU8.set(new Uint8Array(msg.rgba), inPtr);

  // Copy picker params
  const ppLen = msg.pickerParams.length;
  const ppPtr = m._malloc(ppLen * 8);
  for (let i = 0; i < ppLen; i++) {
    m.setValue(ppPtr + i * 8, msg.pickerParams[i], "double");
  }

  // Test 1: run_fill (final only)
  {
    const outPtrPtr = m._malloc(4);
    const outSizePtr = m._malloc(4);
    const rc = m._run_fill(
      inPtr, msg.width, msg.height,
      msg.seedX, msg.seedY,
      msg.tolerance, 0, // frameFreq=0
      msg.algo, msg.picker,
      ppPtr, ppLen,
      outPtrPtr, outSizePtr
    );
    if (rc !== 0) throw new Error("run_fill failed: " + rc);
    const outPtr = m.getValue(outPtrPtr, "*");
    const outData = m.HEAPU8.slice(outPtr, outPtr + pixelBytes);

    // Verify seed pixel is green
    const off = (msg.seedY * msg.width + msg.seedX) * 4;
    if (outData[off+1] !== 255) throw new Error("Seed pixel not green: g=" + outData[off+1]);

    m._free_buffer(outPtr);
    m._free(outPtrPtr);
    m._free(outSizePtr);
    parentPort.postMessage("run_fill OK");
  }

  // Test 2: fill_create (with frames)
  {
    const handle = m._fill_create(
      inPtr, msg.width, msg.height,
      msg.seedX, msg.seedY,
      msg.tolerance, msg.frameFreq,
      msg.algo, msg.picker,
      ppPtr, ppLen,
      msg.maxFrames
    );
    if (!handle) throw new Error("fill_create returned null");

    const count = m._fill_frame_count(handle);
    if (count <= 0) throw new Error("No frames: " + count);

    // Pull each frame
    for (let i = 0; i < count; i++) {
      const fptr = m._fill_get_frame(handle, i);
      if (!fptr) throw new Error("Frame " + i + " is null");
      // Copy to verify readability
      const _data = m.HEAPU8.slice(fptr, fptr + pixelBytes);
    }

    m._fill_destroy(handle);
    parentPort.postMessage("fill_create OK, " + count + " frames");
  }

  m._free(inPtr);
  m._free(ppPtr);

  parentPort.postMessage("ALL_DONE");
}

run().catch(e => {
  parentPort.postMessage("ERROR: " + e.message);
});
