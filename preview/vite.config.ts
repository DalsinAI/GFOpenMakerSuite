// The bench's build. The shelf is fetched from /library/, which the dev
// server serves from here; a build copies it beside the page.
// MIT, Copyright (c) 2026 AmiCradle Team.
import { cpSync } from "node:fs";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwind from "@tailwindcss/vite";

export default defineConfig({
  publicDir: false,
  plugins: [
    react(),
    tailwind(),
    { name: "shelf", closeBundle: () => cpSync("library", "dist/library", { recursive: true }) },
  ],
  server: { port: 5199, host: "127.0.0.1" },
});
