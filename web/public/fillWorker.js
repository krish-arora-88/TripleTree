// Classic Web Worker — loads triplefill WASM module and runs flood fills.
"use strict";

var wasmModule = null;

var ERROR_CODES = {
  "-1": "Invalid arguments",
  "-2": "Fill produced no result (seed out of bounds?)",
  "-3": "Out of memory — try a smaller image or fewer frames",
  "-4": "Image too large (max 4096×4096)",
  "-5": "Unexpected internal error",
};

function describeError(rc) {
  return ERROR_CODES[String(rc)] || "Unknown error (code " + rc + ")";
}

function loadModule() {
  if (wasmModule) return Promise.resolve(wasmModule);

  var base = self.location.href.replace(/\/[^/]*$/, "/");
  if (base.includes("/assets/")) {
    base = base.replace(/\/assets\/$/, "/");
  }
  importScripts(base + "wasm/triplefill.js");

  return TriplefillModule({
    locateFile: function (path) {
      return base + "wasm/" + path;
    },
  }).then(function (m) {
    wasmModule = m;
    return m;
  });
}

self.onmessage = function (e) {
  var msg = e.data;
  if (msg.type !== "fill") return;

  var t0 = performance.now();

  loadModule()
    .then(function (m) {
      var pixelBytes = msg.width * msg.height * 4;

      var inPtr = m._malloc(pixelBytes);
      if (!inPtr) throw new Error("Failed to allocate input buffer (" + pixelBytes + " bytes)");
      m.HEAPU8.set(new Uint8Array(msg.rgba), inPtr);

      var ppLen = msg.pickerParams.length;
      var ppPtr = m._malloc(ppLen * 8);
      for (var i = 0; i < ppLen; i++) {
        m.setValue(ppPtr + i * 8, msg.pickerParams[i], "double");
      }

      var wantFrames = msg.frameFreq > 0 && msg.maxFrames !== 0;
      var frames = [];
      var frameCount = 0;

      if (wantFrames) {
        var handle = m._fill_create(
          inPtr, msg.width, msg.height,
          msg.seedX, msg.seedY,
          msg.tolerance, msg.frameFreq,
          msg.algo, msg.picker,
          ppPtr, ppLen, msg.maxFrames
        );

        if (!handle) {
          // Retry without frames (final only) as fallback
          var outPtrPtr = m._malloc(4);
          var outSizePtr = m._malloc(4);
          var rc = m._run_fill(
            inPtr, msg.width, msg.height,
            msg.seedX, msg.seedY,
            msg.tolerance, 0,
            msg.algo, msg.picker,
            ppPtr, ppLen, outPtrPtr, outSizePtr
          );
          if (rc !== 0) throw new Error(describeError(rc));
          var outPtr = m.getValue(outPtrPtr, "*");
          frames = [m.HEAPU8.slice(outPtr, outPtr + pixelBytes).buffer];
          frameCount = 1;
          m._free_buffer(outPtr);
          m._free(outPtrPtr);
          m._free(outSizePtr);
        } else {
          frameCount = m._fill_frame_count(handle);
          for (var i = 0; i < frameCount; i++) {
            var framePtr = m._fill_get_frame(handle, i);
            if (framePtr) {
              frames.push(m.HEAPU8.slice(framePtr, framePtr + pixelBytes).buffer);
            }
          }
          m._fill_destroy(handle);
          frameCount = frames.length;
        }
      } else {
        var outPtrPtr = m._malloc(4);
        var outSizePtr = m._malloc(4);
        var rc = m._run_fill(
          inPtr, msg.width, msg.height,
          msg.seedX, msg.seedY,
          msg.tolerance, msg.frameFreq,
          msg.algo, msg.picker,
          ppPtr, ppLen, outPtrPtr, outSizePtr
        );
        if (rc !== 0) throw new Error(describeError(rc));
        var outPtr = m.getValue(outPtrPtr, "*");
        frames = [m.HEAPU8.slice(outPtr, outPtr + pixelBytes).buffer];
        frameCount = 1;
        m._free_buffer(outPtr);
        m._free(outPtrPtr);
        m._free(outSizePtr);
      }

      m._free(inPtr);
      m._free(ppPtr);

      var elapsed = performance.now() - t0;
      self.postMessage(
        { type: "result", frames: frames, width: msg.width,
          height: msg.height, frameCount: frameCount, timeMs: Math.round(elapsed) },
        frames
      );
    })
    .catch(function (err) {
      self.postMessage({
        type: "error",
        message: err instanceof Error ? err.message : String(err),
      });
    });
};
