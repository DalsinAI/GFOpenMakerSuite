// The bench in a browser: npm install, then npm run dev. MIT, Copyright (c) 2026 AmiCradle Team.
/// <reference types="vite/client" />
import { createRoot } from "react-dom/client";
import "./styles.css";
import { Studio } from "./modeller/studio";

createRoot(document.getElementById("root")!).render(<Studio />);
