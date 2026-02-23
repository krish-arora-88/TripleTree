import { defineConfig } from "vite";

export default defineConfig({
  base: process.env.VITE_BASE ?? "./",
  build: {
    outDir: "dist",
    assetsInlineLimit: 0,
  },
  server: {
    headers: {
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
  },
});
