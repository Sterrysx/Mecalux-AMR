import { useEffect, useMemo, useRef, useState } from "react";

type Meta = { name: string; width: number; height: number; cell_size_m: number };
type Packet = { id: string; x: number; y: number; weight?: number };
type Charger = { id: string; x: number; y: number; capacity?: number };
type DistFile = {
  meta: Meta;
  legend?: Record<string,string>;
  ascii_map: string[]; // top-first
  objects?: { packets?: Packet[]; chargers?: Charger[] };
  notes?: string;
};

// Brush definitions
type BrushKey = "EMPTY" | "PATH" | "OBSTACLE" | "RESTRICTED" | "CHARGER" | "PACKET";
const BRUSH_TO_CHAR: Record<BrushKey, string> = {
  EMPTY: ".",
  PATH:  "g",      // generic path, green
  OBSTACLE: "#",
  RESTRICTED: "X",
  CHARGER: "C",
  PACKET:  "P",
};

const CELL_COLORS: Record<string,string> = {
  "X":"#7b1113",
  "#":"#9ca3af",
  "C":"#1e90ff",
  "P":"#8b5a2b",
  "b":"#2b6ef3",
  "g":"#3db86b",
  "r":"#e04e4e",
  ".":"#ffffff",
};

export default function App() {
  const [data, setData] = useState<DistFile | null>(null);
  const [grid, setGrid] = useState<string[][]>([]);
  const [brush, setBrush] = useState<BrushKey>("PATH");
  const [err, setErr] = useState<string | null>(null);
  const painting = useRef(false);

  // Load initial JSON (from public)
  useEffect(() => {
    fetch("/distributions/Distribution1.json")
      .then(r => r.json())
      .then((d: DistFile) => {
        const W = d.meta.width, H = d.meta.height;
        // normalize ascii rows (top-first, no spaces)
        const rows = d.ascii_map.map(r => r.replace(/\s+/g,""));
        // convert to bottom-left origin 2D grid[y][x]
        const g: string[][] = Array.from({length: H}, () => Array(W).fill("."));
        for (let r = 0; r < rows.length; r++) {
          const y = (H - 1) - r;
          for (let x = 0; x < Math.min(W, rows[r].length); x++) g[y][x] = rows[r][x];
        }
        setData(d);
        setGrid(g);
      })
      .catch(e => setErr(String(e)));
  }, []);

  const W = data?.meta.width ?? 0;
  const H = data?.meta.height ?? 0;

  // in-memory paint
  function paint(x:number, y:number, value:string) {
    setGrid(prev => {
      const next = prev.map(r => r.slice());
      next[y][x] = value;
      return next;
    });
  }

  // UI handlers
  function handleDown(x:number, y:number) {
    painting.current = true;
    paint(x,y, BRUSH_TO_CHAR[brush]);
  }
  function handleEnter(x:number, y:number) {
    if (painting.current) paint(x,y, BRUSH_TO_CHAR[brush]);
  }
  function handleUp() { painting.current = false; }

  // Build a fresh JSON and download it
  function exportJSON() {
    if (!data) return;
    // convert bottom-left grid back to top-first ascii_map
    const rowsTop: string[] = [];
    for (let r = H-1; r >= 0; r--) {
      rowsTop.push(grid[r].join(""));
    }
    // extract objects from grid
    const chargers: Charger[] = [];
    const packets: Packet[] = [];
    let ci = 1, pi = 1;
    for (let y = 0; y < H; y++) {
      for (let x = 0; x < W; x++) {
        const ch = grid[y][x];
        if (ch === "C") chargers.push({ id: `C${ci++}`, x, y, capacity: 1 });
        if (ch === "P") packets.push({ id: `P${pi++}`, x, y, weight: 1 });
      }
    }
    const out: DistFile = {
      meta: data.meta,
      legend: data.legend ?? {
        "X":"restricted", "#":"obstacle", "C":"charger", "P":"packet",
        "b":"path_blue (1.5 m/s)", "g":"path_green (1.25 m/s)", "r":"path_red (1 m/s)", ".":"empty",
      },
      ascii_map: rowsTop,
      objects: { chargers, packets },
      notes: data.notes ?? "",
    };
    const blob = new Blob([JSON.stringify(out, null, 2)], { type: "application/json" });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = `${data.meta.name || "distribution"}-edited.json`;
    a.click();
    URL.revokeObjectURL(a.href);
  }

  // derived dims
  const CELL = 40;
  const svgW = W * CELL;
  const svgH = H * CELL;

  if (err) return <div className="p-6 text-red-600">Error: {err}</div>;
  if (!data) return <div className="p-6 text-slate-600">Loading…</div>;

  return (
    <div className="min-h-full p-6 bg-slate-50 text-slate-900 select-none" onMouseUp={handleUp} onMouseLeave={handleUp}>
      <div className="mx-auto max-w-[1100px] space-y-4">
        <header className="flex flex-wrap items-end justify-between gap-4">
          <div>
            <h1 className="text-2xl font-semibold">
              {data.meta.name} <span className="text-slate-500">({W}×{H})</span>
            </h1>
            <p className="text-sm text-slate-600">cell size: {data.meta.cell_size_m} m</p>
          </div>
          <div className="flex items-center gap-2">
            <button
              onClick={exportJSON}
              className="rounded-md bg-slate-900 px-3 py-2 text-white hover:bg-slate-800"
            >
              Export JSON
            </button>
          </div>
        </header>

        {/* Brush toolbar */}
        <div className="flex flex-wrap items-center gap-2 rounded-lg border border-slate-300 bg-white p-3">
          {([
            ["PATH","g","#3db86b"],
            ["OBSTACLE","#","#9ca3af"],
            ["RESTRICTED","X","#7b1113"],
            ["CHARGER","C","#1e90ff"],
            ["PACKET","P","#8b5a2b"],
            ["EMPTY",".","#ffffff"],
          ] as [BrushKey,string,string][])
            .map(([key, ch, color]) => (
            <button
              key={key}
              onClick={() => setBrush(key)}
              className={`flex items-center gap-2 rounded-md border px-3 py-1 text-sm
                ${brush===key ? "border-slate-900" : "border-slate-300 hover:border-slate-400"}`}
              title={`${key} (${ch})`}
            >
              <span className="inline-block h-4 w-4 rounded border border-slate-300" style={{ background: color }} />
              <span>{key}</span>
            </button>
          ))}
        </div>

        {/* Map */}
        <div className="relative overflow-auto rounded-lg border border-slate-300 bg-white p-3">
          <svg width={svgW} height={svgH} className="block">
            {/* draw cells */}
            {grid.map((row, y) => row.map((ch, x) => (
              <rect
                key={`${x}-${y}`}
                x={x*CELL}
                y={(H-1-y)*CELL}
                width={CELL}
                height={CELL}
                fill={CELL_COLORS[ch] ?? "#eee"}
                stroke="#cbd5e1"
                strokeWidth={1}
                onMouseDown={() => handleDown(x,y)}
                onMouseEnter={() => handleEnter(x,y)}
              />
            )))}

            {/* overlays for chargers/packets (optional, we show letters on top) */}
            {grid.map((row, y) => row.map((ch, x) => (
              (ch==="C" || ch==="P") ? (
                <text
                  key={`t-${x}-${y}`}
                  x={x*CELL + CELL*0.5}
                  y={(H-1-y)*CELL + CELL*0.62}
                  fontSize={Math.max(10, CELL*0.36)}
                  textAnchor="middle"
                  fill={ch==="C" ? "#1e90ff" : "#5b3b1a"}
                  style={{ pointerEvents:"none" }}
                >{ch}</text>
              ) : null
            )))}
          </svg>
        </div>
      </div>
    </div>
  );
}
