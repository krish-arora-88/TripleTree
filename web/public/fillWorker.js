// Classic Web Worker — loads triplefill WASM module and runs flood fills.
//
// maxFrames/frameFreq contract (authoritative):
//   frameFreq == 0  → final-only (1 frame, no intermediates)
//   maxFrames == 0  → unlimited intermediate frames (clamped by memory estimate)
//   maxFrames > 0   → cap intermediate frames to that number; final always appended
"use strict";

var wasmModule = null;

// 256 MB budget for frame storage (tunable)
var SAFE_LIMIT_BYTES = 256 * 1024 * 1024;

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

function getWasmErrorMessage(m) {
  if (typeof m._fill_last_error_code !== "function") return null;
  var code = m._fill_last_error_code();
  if (code === 0) return null;
  if (typeof m._fill_last_error_message !== "function") return "Error code " + code;
  var ptr = m._fill_last_error_message();
  if (ptr) {
    var msg = "";
    for (var i = 0; i < 256; i++) {
      var ch = m.HEAPU8[ptr + i];
      if (ch === 0) break;
      msg += String.fromCharCode(ch);
    }
    return msg;
  }
  return "Error code " + code;
}

self.onmessage = function (e) {
  var msg = e.data;
  if (msg.type !== "fill") return;

  var t0 = performance.now();

  loadModule()
    .then(function (m) {
      var width = msg.width;
      var height = msg.height;
      var pixelBytes = width * height * 4;
      var bytesPerFrame = pixelBytes;
      var warnings = [];

      // ---- Preflight memory estimate + auto-clamp ----
      var wantFrames = msg.frameFreq > 0;
      var requestedMaxFrames = msg.maxFrames;
      var effectiveMaxFrames = requestedMaxFrames;

      if (wantFrames) {
        if (requestedMaxFrames === 0) {
          // Unlimited → clamp to safe memory budget
          var clamp = Math.floor(SAFE_LIMIT_BYTES / bytesPerFrame) - 1;
          effectiveMaxFrames = Math.max(1, clamp);
          if (clamp <= 0) {
            wantFrames = false;
            warnings.push(
              "Image too large for frame capture (" +
                Math.round(bytesPerFrame / 1024) +
                " KB/frame). Showing final image only."
            );
          }
        } else {
          var estimatedBytes = bytesPerFrame * (requestedMaxFrames + 1);
          if (estimatedBytes > SAFE_LIMIT_BYTES) {
            var clampedMax = Math.max(
              1,
              Math.floor(SAFE_LIMIT_BYTES / bytesPerFrame) - 1
            );
            effectiveMaxFrames = Math.min(requestedMaxFrames, clampedMax);
            warnings.push(
              "Reduced max frames from " +
                requestedMaxFrames +
                " to " +
                effectiveMaxFrames +
                " to stay within memory limits."
            );
          }
        }
      }

      var estimatedBytes = wantFrames
        ? bytesPerFrame * (effectiveMaxFrames + 1)
        : bytesPerFrame;

      // ---- Allocate input ----
      var inPtr = m._malloc(pixelBytes);
      if (!inPtr)
        throw new Error(
          "Failed to allocate input buffer (" + pixelBytes + " bytes)"
        );
      m.HEAPU8.set(new Uint8Array(msg.rgba), inPtr);

      var ppLen = msg.pickerParams.length;
      var ppPtr = m._malloc(ppLen * 8);
      for (var i = 0; i < ppLen; i++) {
        m.setValue(ppPtr + i * 8, msg.pickerParams[i], "double");
      }

      var frames = [];
      var frameCount = 0;
      var filledPixels = 0;
      var status = "ok";

      if (wantFrames) {
        var handle = m._fill_create(
          inPtr,
          width,
          height,
          msg.seedX,
          msg.seedY,
          msg.tolerance,
          msg.frameFreq,
          msg.algo,
          msg.picker,
          ppPtr,
          ppLen,
          effectiveMaxFrames
        );

        if (!handle) {
          // Frame capture failed — get error from WASM
          var wasmErr = getWasmErrorMessage(m);
          status = "warn";
          warnings.push(
            "Frame capture disabled" +
              (wasmErr ? " (" + wasmErr + ")" : " (out of memory)") +
              ". Showing final image only."
          );

          // Fallback to final-only
          var outPtrPtr = m._malloc(4);
          var outSizePtr = m._malloc(4);
          var rc = m._run_fill(
            inPtr,
            width,
            height,
            msg.seedX,
            msg.seedY,
            msg.tolerance,
            0,
            msg.algo,
            msg.picker,
            ppPtr,
            ppLen,
            outPtrPtr,
            outSizePtr
          );
          if (rc !== 0) throw new Error(describeError(rc));
          var outPtr = m.getValue(outPtrPtr, "*");
          frames = [m.HEAPU8.slice(outPtr, outPtr + pixelBytes).buffer];
          frameCount = 1;
          m._free_buffer(outPtr);
          m._free(outPtrPtr);
          m._free(outSizePtr);
        } else {
          if (typeof m._fill_get_filled_pixels === "function") {
            filledPixels = m._fill_get_filled_pixels(handle);
          }
          frameCount = m._fill_frame_count(handle);
          for (var i = 0; i < frameCount; i++) {
            var framePtr = m._fill_get_frame(handle, i);
            if (framePtr) {
              frames.push(
                m.HEAPU8.slice(framePtr, framePtr + pixelBytes).buffer
              );
            }
          }
          m._fill_destroy(handle);
          frameCount = frames.length;
        }
      } else {
        // Final-only path
        var outPtrPtr = m._malloc(4);
        var outSizePtr = m._malloc(4);
        var rc = m._run_fill(
          inPtr,
          width,
          height,
          msg.seedX,
          msg.seedY,
          msg.tolerance,
          msg.frameFreq,
          msg.algo,
          msg.picker,
          ppPtr,
          ppLen,
          outPtrPtr,
          outSizePtr
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
        {
          type: "result",
          frames: frames,
          width: width,
          height: height,
          frameCount: frameCount,
          timeMs: Math.round(elapsed),
          status: status,
          warning: warnings.length > 0 ? warnings.join(" ") : undefined,
          memory: {
            bytesPerFrame: bytesPerFrame,
            safeLimitBytes: SAFE_LIMIT_BYTES,
            requestedMaxFrames: requestedMaxFrames,
            effectiveMaxFrames: effectiveMaxFrames,
            estimatedBytes: estimatedBytes,
          },
          stats: {
            framesCaptured: frameCount,
            filledPixels: filledPixels,
            algo: msg.algo === 1 ? "DFS" : "BFS",
          },
        },
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
