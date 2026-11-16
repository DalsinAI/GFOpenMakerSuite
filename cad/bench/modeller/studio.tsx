import { useEffect, useRef, useState, type ReactNode } from "react";
import {
  Box,
  Circle,
  Cone,
  Copy,
  Cylinder,
  Download,
  Eye,
  EyeOff,
  FlipHorizontal2,
  Focus,
  Magnet,
  Move3d,
  Pyramid,
  RectangleHorizontal,
  Redo2,
  Rotate3d,
  RotateCw,
  Ruler,
  Scaling,
  Slice,
  Torus,
  Trash2,
  Undo2,
  Upload,
} from "lucide-react";
import { Bench, type BenchSnapshot, type Shade, type Tool } from "./bench";
import { METALS, SHELF } from "./library";
import type { SolidKind } from "./solids";

const TOOLS: { id: Tool; label: string; icon: typeof Move3d }[] = [
  { id: "translate", label: "Move", icon: Move3d },
  { id: "rotate", label: "Turn", icon: Rotate3d },
  { id: "scale", label: "Scale", icon: Scaling },
];

const SHADES: { id: Shade; label: string }[] = [
  { id: "metal", label: "Metal" },
  { id: "clay", label: "Clay" },
  { id: "wire", label: "Wire" },
];

const SOLIDS: { id: SolidKind; label: string; icon: typeof Box }[] = [
  { id: "box", label: "Box", icon: Box },
  { id: "cylinder", label: "Cylinder", icon: Cylinder },
  { id: "sphere", label: "Sphere", icon: Circle },
  { id: "cone", label: "Cone", icon: Cone },
  { id: "torus", label: "Torus", icon: Torus },
  { id: "wedge", label: "Wedge", icon: Pyramid },
  { id: "slot", label: "Slot", icon: RectangleHorizontal },
];

function saveFile(filename: string, buffer: ArrayBuffer) {
  const blob = new Blob([buffer], { type: "model/stl" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = filename;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

function fmt(n: number) {
  if (!Number.isFinite(n)) return "";
  const a = Math.abs(n);
  if (a >= 100) return n.toFixed(1);
  return n.toFixed(2);
}

function Num({
  label,
  value,
  onCommit,
}: {
  label: string;
  value: number;
  onCommit: (n: number) => void;
}) {
  const [text, setText] = useState(fmt(value));
  const [focus, setFocus] = useState(false);
  useEffect(() => {
    if (!focus) setText(fmt(value));
  }, [value, focus]);
  return (
    <label className="num">
      {label}
      <input
        inputMode="decimal"
        value={text}
        onFocus={() => setFocus(true)}
        onChange={(e) => setText(e.target.value)}
        onBlur={() => {
          setFocus(false);
          const n = Number(text);
          if (Number.isFinite(n)) onCommit(n);
          else setText(fmt(value));
        }}
        onKeyDown={(e) => {
          if (e.key === "Enter") (e.currentTarget as HTMLInputElement).blur();
        }}
      />
    </label>
  );
}

function Group({ title, children }: { title: string; children: ReactNode }) {
  return (
    <fieldset className="group">
      <legend>{title}</legend>
      {children}
    </fieldset>
  );
}

function Field({ label, children }: { label: string; children: ReactNode }) {
  return (
    <>
      <span className="form-label">{label}</span>
      <div className="form-control">{children}</div>
    </>
  );
}

export function Studio() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const benchRef = useRef<Bench | null>(null);
  const fileRef = useRef<HTMLInputElement>(null);
  const [snap, setSnap] = useState<BenchSnapshot | null>(null);
  const [hot, setHot] = useState(false);
  const [drill, setDrill] = useState("6");
  const [holes, setHoles] = useState("4");
  const [pcd, setPcd] = useState("16");
  const [cbore, setCbore] = useState("12");
  const [cdeep, setCdeep] = useState("4");
  const [pitch, setPitch] = useState("8");
  const [height, setHeight] = useState("8");
  const [wide, setWide] = useState("20");
  const [deep, setDeep] = useState("14");
  const [copies, setCopies] = useState("4");
  const [gap, setGap] = useState("6");
  const [pullMm, setPullMm] = useState("4");
  const [layerH, setLayerH] = useState("0.2");

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const bench = new Bench(canvas, setSnap);
    benchRef.current = bench;
    void bench.boot();
    return () => {
      bench.dispose();
      benchRef.current = null;
    };
  }, []);

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const bench = benchRef.current;
      if (!bench) return;
      const tag = (e.target as HTMLElement | null)?.tagName;
      if (tag === "INPUT" || tag === "TEXTAREA") return;
      const key = e.key.toLowerCase();
      if ((e.metaKey || e.ctrlKey) && key === "z") {
        e.preventDefault();
        if (e.shiftKey) bench.redo();
        else bench.undo();
        return;
      }
      if ((e.metaKey || e.ctrlKey) && key === "d") {
        e.preventDefault();
        bench.duplicate();
        return;
      }
      if ((e.metaKey || e.ctrlKey) && key === "y") {
        e.preventDefault();
        bench.redo();
        return;
      }
      if (e.metaKey || e.ctrlKey || e.altKey) return;
      if (key === "g") bench.setTool("translate");
      else if (key === "r") bench.setTool("rotate");
      else if (key === "s") bench.setTool("scale");
      else if (key === "f") bench.frame(false);
      else if (key === "m") bench.setMeasuring(!bench.snapshot.measuring);
      else if (key === "c") bench.setSection(!bench.snapshot.section);
      else if (key === "i") bench.solo();
      else if (key === "[") bench.nudgeSection(-1);
      else if (key === "]") bench.nudgeSection(1);
      else if (key === "p") bench.setPull(!bench.snapshot.pulling);
      else if (key === "delete" || key === "backspace") {
        e.preventDefault();
        bench.deleteSelected();
      } else if (key === "escape") {
        bench.setMeasuring(false);
        bench.setSketch(null);
        bench.setPull(false);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  const bench = benchRef.current;
  const selected = snap?.parts.find((p) => p.selected) ?? null;
  const cm3 = selected ? selected.volume / 1000 : 0;
  const grams = cm3 * 1.24;

  const takeFiles = (files: FileList | File[]) => {
    const list = [...files].filter((f) => /\.stl$/i.test(f.name) || f.type === "model/stl");
    for (const file of list) {
      file.arrayBuffer().then((buf) => benchRef.current?.openBuffer(buf, file.name));
    }
  };

  return (
    <div className="studio">
      <header className="caption">
        <div className="brand">
          <strong>GFOpenMake</strong>
          <span>Mesh workshop · 220 mm plate</span>
        </div>
      </header>
      <div className="body">
        <aside className="shelf">
          <Group title="Tools">
            <div className="row">
              {TOOLS.map((t) => (
                <button
                  key={t.id}
                  type="button"
                  className={snap?.tool === t.id && !snap.measuring ? "tool on" : "tool"}
                  onClick={() => bench?.setTool(t.id)}
                >
                  <t.icon className="icon" /> {t.label}
                </button>
              ))}
              <button
                type="button"
                className={snap?.measuring ? "tool on" : "tool"}
                onClick={() => bench?.setMeasuring(!snap?.measuring)}
              >
                <Ruler className="icon" /> Measure
              </button>
              <button
                type="button"
                className={snap?.snap ? "tool on" : "tool"}
                onClick={() => bench?.setSnap(!snap?.snap)}
              >
                <Magnet className="icon" /> Snap
              </button>
            </div>
            <p className="quiet">G move, R turn, S scale, P pull a face, M measure.</p>
          </Group>
          <Group title="Picture">
            <div className="row">
              {SHADES.map((s) => (
                <button
                  key={s.id}
                  type="button"
                  className={snap?.shade === s.id ? "tool on" : "tool"}
                  onClick={() => bench?.setShade(s.id)}
                >
                  {s.label}
                </button>
              ))}
            </div>
            <div className="row">
              <button type="button" className="tool" onClick={() => bench?.look("top")}>
                Top
              </button>
              <button type="button" className="tool" onClick={() => bench?.look("front")}>
                Front
              </button>
              <button type="button" className="tool" onClick={() => bench?.look("side")}>
                Side
              </button>
              <button type="button" className="tool" onClick={() => bench?.frame(false)}>
                <Focus className="icon" /> Frame
              </button>
            </div>
            <div className="row">
              <button
                type="button"
                className={snap?.section ? "tool on" : "tool"}
                onClick={() => bench?.setSection(!snap?.section)}
              >
                <Slice className="icon" /> Section
              </button>
              <button type="button" className="tool" disabled={snap?.busy} onClick={() => void bench?.split()}>
                Split
              </button>
            </div>
            <p className="quiet">C toggles the section. [ and ] move the plane.</p>
          </Group>
          <Group title="Extrude">
            <p className="quiet">
              Add sits on the bed. On joins a boss to the selected part. Cut takes that depth off its top. A round uses
              width as the diameter.
            </p>
            <div className="form">
              <Field label="Height">
                <input
                  className="field"
                  inputMode="decimal"
                  aria-label="Extrude height in millimetres"
                  value={height}
                  onChange={(e) => {
                    setHeight(e.target.value);
                    const n = Number(e.target.value);
                    if (Number.isFinite(n)) bench?.setSketchHeight(n);
                  }}
                />
              </Field>
              <Field label="Width">
                <input
                  className="field"
                  inputMode="decimal"
                  aria-label="Extrude width in millimetres"
                  value={wide}
                  onChange={(e) => setWide(e.target.value)}
                />
              </Field>
              <Field label="Depth">
                <input
                  className="field"
                  inputMode="decimal"
                  aria-label="Extrude depth in millimetres"
                  value={deep}
                  onChange={(e) => setDeep(e.target.value)}
                />
              </Field>
            </div>
            <div className="row">
              <button
                type="button"
                className={snap?.sketchOp === "cut" || snap?.sketchOp === "top" ? "tool" : "tool on"}
                onClick={() => bench?.setSketchOp("add")}
              >
                Add
              </button>
              <button
                type="button"
                className={snap?.sketchOp === "top" ? "tool on" : "tool"}
                onClick={() => bench?.setSketchOp("top")}
              >
                On
              </button>
              <button
                type="button"
                className={snap?.sketchOp === "cut" ? "tool on" : "tool"}
                onClick={() => bench?.setSketchOp("cut")}
              >
                Cut
              </button>
              <button
                type="button"
                className="tool"
                onClick={() =>
                  bench?.place(
                    snap?.sketch === "circle" ? "circle" : snap?.sketch === "slot" ? "slot" : "rect",
                    Number(wide),
                    Number(deep),
                  )
                }
              >
                Place
              </button>
            </div>
            <div className="row">
              <button
                type="button"
                className={snap?.sketch === "rect" ? "solid on" : "solid"}
                onClick={() => bench?.setSketch("rect")}
              >
                <Box className="icon" /> Block
              </button>
              <button
                type="button"
                className={snap?.sketch === "circle" ? "solid on" : "solid"}
                onClick={() => bench?.setSketch("circle")}
              >
                <Circle className="icon" /> Round
              </button>
              <button
                type="button"
                className={snap?.sketch === "slot" ? "solid on" : "solid"}
                onClick={() => bench?.setSketch("slot")}
              >
                Keyway
              </button>
              <button type="button" className="solid" onClick={() => bench?.addWasher()}>
                Washer
              </button>
            </div>
          </Group>
          <Group title="Make">
            <div className="solids">
              {SOLIDS.map((s) => (
                <button key={s.id} type="button" className="solid" onClick={() => bench?.addSolid(s.id)}>
                  <s.icon className="icon" /> {s.label}
                </button>
              ))}
            </div>
          </Group>
          <Group title="Library">
            <div className="shelf-list">
              {SHELF.map((item) => (
                <button
                  key={item.file}
                  type="button"
                  className="shelf-item"
                  onClick={() => void bench?.openUrl(item.file, item.name)}
                >
                  <b>{item.name}</b>
                  <small>{item.note}</small>
                </button>
              ))}
            </div>
            <p className="credit">NASA models are public domain. The Utah teapot solid is CC0. Workshop parts are made here.</p>
          </Group>
        </aside>
        <div
          className={hot ? "viewport hot" : "viewport"}
          onDragOver={(e) => {
            e.preventDefault();
            setHot(true);
          }}
          onDragLeave={() => setHot(false)}
          onDrop={(e) => {
            e.preventDefault();
            setHot(false);
            if (e.dataTransfer.files.length) takeFiles(e.dataTransfer.files);
          }}
        >
          <canvas ref={canvasRef} />
          <div className="hud">
            <span>Left drag orbits</span>
            <span>Right drag pans</span>
            <span>Wheel zooms</span>
            <span>220 mm bed · Z up</span>
          </div>
          <div className="float-bars">
            {snap?.section && (
              <label className="section-bar">
                Section {snap.sectionZ.toFixed(1)} mm
                <input
                  type="range"
                  min={0}
                  max={Math.max(snap.sectionMax, snap.sectionZ, 1)}
                  step={0.5}
                  value={Math.min(snap.sectionZ, Math.max(snap.sectionMax, snap.sectionZ))}
                  onChange={(e) => benchRef.current?.setSectionZ(Number(e.target.value))}
                />
              </label>
            )}
            {snap?.layer && snap.layerCount > 0 && (
              <label className="section-bar">
                Layer {snap.layerIndex + 1} of {snap.layerCount}
                <input
                  type="range"
                  min={0}
                  max={Math.max(0, snap.layerCount - 1)}
                  step={1}
                  value={snap.layerIndex}
                  onChange={(e) => benchRef.current?.setLayerIndex(Number(e.target.value))}
                />
              </label>
            )}
          </div>
        </div>
        <aside className="inspector">
          <Group title="Parts">
            {snap?.parts.length ? (
              <div className="part-list">
                {snap.parts.map((part) => (
                  <div className="part-row" key={part.id}>
                    <button
                      type="button"
                      className={part.selected ? "part on" : "part"}
                      onClick={() => bench?.select(part.id)}
                    >
                      <i className="dot" style={{ background: part.color }} />
                      <span>{part.name}</span>
                      <small>{part.tris.toLocaleString()}</small>
                    </button>
                    <button
                      type="button"
                      className="eye"
                      aria-label={part.visible ? "Hide " + part.name : "Show " + part.name}
                      onClick={() => bench?.toggleVisible(part.id)}
                    >
                      {part.visible ? <Eye className="icon" /> : <EyeOff className="icon" />}
                    </button>
                  </div>
                ))}
              </div>
            ) : (
              <p className="empty">The bed is empty. Drop an STL, or pick one from the library.</p>
            )}
          </Group>

          {selected && (
            <>
              <Group title="Selected">
                <input
                  className="field"
                  aria-label="Part name"
                  defaultValue={selected.name}
                  key={selected.id + selected.name}
                  onBlur={(e) => bench?.rename(selected.id, e.target.value)}
                />
                <div className="swatches">
                  {METALS.map((hex) => (
                    <button
                      key={hex}
                      type="button"
                      className="swatch"
                      aria-label={"Colour " + hex}
                      onClick={() => bench?.setColor(hex)}
                    >
                      <i style={{ background: hex }} />
                    </button>
                  ))}
                </div>
                <dl className="facts">
                  <div>
                    <dt>PLA, solid</dt>
                    <dd>
                      {cm3 >= 100 ? cm3.toFixed(0) : cm3.toFixed(2)} cm³
                      {cm3 > 0 && cm3 < 5000 ? ` · ${grams.toFixed(grams >= 10 ? 0 : 1)} g` : ""}
                    </dd>
                  </div>
                  <div>
                    <dt>Triangles</dt>
                    <dd>{selected.tris.toLocaleString()}</dd>
                  </div>
                </dl>
                {selected.offBed && <p className="warn">Hangs off the 220 mm bed.</p>}
              </Group>
              <Group title="Size and place">
                <p className="quiet">A new size scales the part evenly. The shape does not stretch.</p>
                <div className="form">
                  <Field label="Size mm">
                    <div className="nums">
                      {(["X", "Y", "Z"] as const).map((axis, i) => (
                        <Num
                          key={axis}
                          label={axis}
                          value={selected.size[i] ?? 0}
                          onCommit={(n) => bench?.fitExtent(i as 0 | 1 | 2, n)}
                        />
                      ))}
                    </div>
                  </Field>
                  <Field label="Position">
                    <div className="nums">
                      {(["X", "Y", "Z"] as const).map((axis, i) => (
                        <Num
                          key={axis}
                          label={axis}
                          value={selected.pos[i] ?? 0}
                          onCommit={(n) => {
                            const next: [number, number, number] = [...selected.pos];
                            next[i] = n;
                            bench?.setPosition(next[0], next[1], next[2]);
                          }}
                        />
                      ))}
                    </div>
                  </Field>
                  <Field label="Rotation">
                    <div className="nums">
                      {(["X", "Y", "Z"] as const).map((axis, i) => (
                        <Num
                          key={axis}
                          label={axis}
                          value={selected.rot[i] ?? 0}
                          onCommit={(n) => {
                            const next: [number, number, number] = [...selected.rot];
                            next[i] = n;
                            bench?.setRotation(next[0], next[1], next[2]);
                          }}
                        />
                      ))}
                    </div>
                  </Field>
                  <Field label="Scale">
                    <div className="nums">
                      {(["X", "Y", "Z"] as const).map((axis, i) => (
                        <Num
                          key={axis}
                          label={axis}
                          value={selected.scale[i] ?? 1}
                          onCommit={(n) => {
                            const next: [number, number, number] = [...selected.scale];
                            next[i] = n;
                            bench?.setScale(next[0], next[1], next[2]);
                          }}
                        />
                      ))}
                    </div>
                  </Field>
                </div>
              </Group>
              <Group title="Arrange">
                <p className="quiet">Click a face, then Pull. Line and Ring copy the selected part.</p>
                <div className="form">
                  <Field label="Copies">
                    <input className="field" inputMode="numeric" value={copies} onChange={(e) => setCopies(e.target.value)} />
                  </Field>
                  <Field label="Gap mm">
                    <input className="field" inputMode="decimal" value={gap} onChange={(e) => setGap(e.target.value)} />
                  </Field>
                  <Field label="Pull mm">
                    <input className="field" inputMode="decimal" value={pullMm} onChange={(e) => setPullMm(e.target.value)} />
                  </Field>
                </div>
                <div className="row">
                  <button
                    type="button"
                    className={snap?.pulling ? "tool on" : "tool"}
                    onClick={() => bench?.setPull(!snap?.pulling)}
                  >
                    Pull face
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.pullBy(Number(pullMm))}>
                    Pull
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.chamfer(Number(pullMm))}>
                    Chamfer
                  </button>
                </div>
                <div className="row">
                  <button type="button" className="tool" onClick={() => bench?.line(0, Number(copies), Number(gap))}>
                    Line X
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.line(1, Number(copies), Number(gap))}>
                    Line Y
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.ring(Number(copies))}>
                    Ring
                  </button>
                </div>
                <div className="row">
                  <button type="button" className="tool" onClick={() => bench?.drop()}>
                    Drop
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.center()}>
                    Centre
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.lay()}>
                    Lay flat
                  </button>
                  <button type="button" className="tool" disabled={!snap?.stockName} onClick={() => bench?.stack()}>
                    Stack
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.solo()}>
                    Solo
                  </button>
                </div>
                <div className="row">
                  <button type="button" className="tool" onClick={() => bench?.turn("z", 90)}>
                    <RotateCw className="icon" /> 90°
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.duplicate()}>
                    <Copy className="icon" /> Copy
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.repeat()}>
                    ×3
                  </button>
                  <button type="button" className="tool" onClick={() => bench?.mirror()}>
                    <FlipHorizontal2 className="icon" /> Mirror
                  </button>
                  <button type="button" className="tool danger" onClick={() => bench?.deleteSelected()}>
                    <Trash2 className="icon" /> Delete
                  </button>
                </div>
              </Group>
              <Group title="Layers">
                <p className="quiet">Walls of the selected part, one height at a time. No infill and no G-code.</p>
                <div className="form">
                  <Field label="Height">
                    <input
                      className="field"
                      inputMode="decimal"
                      aria-label="Layer height in millimetres"
                      value={layerH}
                      onChange={(e) => {
                        setLayerH(e.target.value);
                        const n = Number(e.target.value);
                        if (Number.isFinite(n) && n > 0) bench?.setLayerHeight(n);
                      }}
                      onBlur={() => {
                        const n = Number(layerH);
                        if (!Number.isFinite(n) || n <= 0) {
                          setLayerH("0.2");
                          return;
                        }
                        const c = Math.min(0.8, Math.max(0.08, n));
                        setLayerH(String(Math.round(c * 1000) / 1000));
                      }}
                    />
                  </Field>
                  <Field label="Show">
                    <button
                      type="button"
                      className={snap?.layer ? "tool on" : "tool"}
                      onClick={() => bench?.setLayer(!snap?.layer)}
                    >
                      {snap?.layer ? "Hide layer" : "Show layer"}
                    </button>
                  </Field>
                  {snap?.layer && snap.layerCount > 0 && (
                    <Field label="Layer">
                      <div className="row">
                        <button
                          type="button"
                          className="tool"
                          disabled={snap.layerIndex <= 0}
                          onClick={() => bench?.setLayerIndex(snap.layerIndex - 1)}
                        >
                          Previous
                        </button>
                        <button
                          type="button"
                          className="tool"
                          disabled={snap.layerIndex >= snap.layerCount - 1}
                          onClick={() => bench?.setLayerIndex(snap.layerIndex + 1)}
                        >
                          Next
                        </button>
                      </div>
                      <p className="quiet">
                        {snap.layerIndex + 1} of {snap.layerCount}
                        {snap.layerMm > 0 ? ` · ${snap.layerMm.toFixed(0)} mm of wall` : ""}
                      </p>
                    </Field>
                  )}
                </div>
              </Group>
              <Group title="Holes and joins">
                <p className="quiet">
                  {snap?.stockName
                    ? `${selected.name} is the tool. ${snap.stockName} is the stock. Limit 2,500 triangles.`
                    : "The part under this one is the stock. Drill bores the selection on its own."}
                </p>
                <div className="form">
                  <Field label="Diameter">
                    <input
                      className="field"
                      inputMode="decimal"
                      aria-label="Hole diameter in millimetres"
                      value={drill}
                      onChange={(e) => setDrill(e.target.value)}
                    />
                  </Field>
                  <Field label="Drill">
                    <button type="button" className="tool" disabled={snap?.busy} onClick={() => void bench?.drill(Number(drill))}>
                      Drill through
                    </button>
                  </Field>
                  <Field label="Holes">
                    <input className="field" inputMode="numeric" aria-label="Number of holes" value={holes} onChange={(e) => setHoles(e.target.value)} />
                  </Field>
                  <Field label="PCD">
                    <input className="field" inputMode="decimal" aria-label="Pitch circle diameter" value={pcd} onChange={(e) => setPcd(e.target.value)} />
                  </Field>
                  <Field label="Circle">
                    <button
                      type="button"
                      className="tool"
                      disabled={snap?.busy}
                      onClick={() => void bench?.boltCircle(Number(holes), Number(drill), Number(pcd))}
                    >
                      Bolt circle
                    </button>
                  </Field>
                  <Field label="Pitch">
                    <input className="field" inputMode="decimal" aria-label="Hole pitch in millimetres" value={pitch} onChange={(e) => setPitch(e.target.value)} />
                  </Field>
                  <Field label="Row">
                    <button
                      type="button"
                      className="tool"
                      disabled={snap?.busy}
                      onClick={() => void bench?.holeRow(Number(holes), Number(drill), Number(pitch))}
                    >
                      Hole row
                    </button>
                  </Field>
                  <Field label="Bore">
                    <input className="field" inputMode="decimal" aria-label="Counterbore diameter" value={cbore} onChange={(e) => setCbore(e.target.value)} />
                  </Field>
                  <Field label="Deep">
                    <input className="field" inputMode="decimal" aria-label="Counterbore depth" value={cdeep} onChange={(e) => setCdeep(e.target.value)} />
                  </Field>
                  <Field label="Head">
                    <div className="row">
                      <button
                        type="button"
                        className="tool"
                        disabled={snap?.busy}
                        onClick={() => void bench?.counterbore(Number(drill), Number(cbore), Number(cdeep))}
                      >
                        Counterbore
                      </button>
                      <button
                        type="button"
                        className="tool"
                        disabled={snap?.busy}
                        onClick={() => void bench?.countersink(Number(drill), Number(cbore))}
                      >
                        Countersink
                      </button>
                    </div>
                  </Field>
                  <Field label="Join">
                    <div className="row">
                      <button
                        type="button"
                        className="tool"
                        disabled={!snap?.stockName || snap.busy}
                        onClick={() => void bench?.combine("union")}
                      >
                        Join
                      </button>
                      <button
                        type="button"
                        className="tool"
                        disabled={!snap?.stockName || snap.busy}
                        onClick={() => void bench?.combine("subtract")}
                      >
                        Cut
                      </button>
                      <button
                        type="button"
                        className="tool"
                        disabled={!snap?.stockName || snap.busy}
                        onClick={() => void bench?.combine("intersect")}
                      >
                        Intersect
                      </button>
                    </div>
                  </Field>
                </div>
              </Group>
            </>
          )}
        </aside>
      </div>
      <footer className="statusbar">
        <p className="status">{snap?.status ?? "Reading the NASA wrench…"}</p>
        <button type="button" className="tool" onClick={() => fileRef.current?.click()}>
          <Upload className="icon" /> Open STL
        </button>
        <button
          type="button"
          className="tool"
          onClick={() => {
            const out = benchRef.current?.exportSelected();
            if (out) saveFile(out.filename, out.buffer);
          }}
          disabled={!selected}
        >
          <Download className="icon" /> Save part
        </button>
        <button type="button" className="tool" onClick={() => bench?.undo()}>
          <Undo2 className="icon" /> Undo
        </button>
        <button type="button" className="tool" onClick={() => bench?.redo()}>
          <Redo2 className="icon" /> Redo
        </button>
        <button
          type="button"
          className="start"
          onClick={() => {
            const out = benchRef.current?.exportAll();
            if (out) saveFile(out.filename, out.buffer);
          }}
          disabled={!snap?.parts.length}
        >
          Save STL
        </button>
      </footer>
      <input
        ref={fileRef}
        type="file"
        accept=".stl,model/stl"
        multiple
        hidden
        onChange={(e) => {
          if (e.target.files) takeFiles(e.target.files);
          e.target.value = "";
        }}
      />
    </div>
  );
}
