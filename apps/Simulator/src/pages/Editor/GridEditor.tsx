import { useRef, useState } from "react";
import { useDistribution } from "../../contexts/DistributionContext";
import { useTheme } from "../../contexts/ThemeContext";

type BrushKey = "EMPTY" | "PATH" | "OBSTACLE" | "RESTRICTED" | "CHARGER" | "PACKET";
const BRUSH_TO_CHAR: Record<BrushKey, string> = {
  EMPTY: ".", PATH: "g", OBSTACLE: "#", RESTRICTED: "X", CHARGER: "C", PACKET: "P",
};

const CELL_COLORS: Record<string,string> = {
  "X":"#7b1113","#":"#9ca3af","C":"#1e90ff","P":"#8b5a2b","b":"#2b6ef3","g":"#3db86b","r":"#e04e4e",".":"#ffffff",
};

export default function GridEditor() {
  const { distribution, grid, updateGrid, saveDistribution, isLoading, error } = useDistribution();
  const { darkMode } = useTheme();
  const [brush, setBrush] = useState<BrushKey>("PATH");
  const painting = useRef(false);

  const W = distribution?.meta.width ?? 0;
  const H = distribution?.meta.height ?? 0;

  const paint = (x: number, y: number, val: string) => {
    const newGrid = grid.map(r => r.slice());
    newGrid[y][x] = val;
    updateGrid(newGrid);
  };

  const exportJSON = async () => {
    try {
      await saveDistribution();
      // The save function handles the download
    } catch (err) {
      console.error('Failed to export:', err);
    }
  };

  const CELL = 40;
  const svgW = W * CELL;
  const svgH = H * CELL;
  
  if (error) return <div className="p-6 text-red-600">Error: {error}</div>;
  if (isLoading || !distribution) return <div className="p-6">Loading…</div>;

  return (
    <div className={`mx-auto max-w-[1100px] p-6 space-y-4 ${darkMode ? 'text-slate-100' : 'text-slate-900'}`} onMouseUp={()=>painting.current=false} onMouseLeave={()=>painting.current=false}>
      <div className="flex items-end justify-between">
        <div>
          <h1 className={`text-2xl font-semibold ${darkMode ? 'text-white' : 'text-slate-900'}`}>{distribution.meta.name} <span className={`${darkMode ? 'text-slate-300' : 'text-slate-500'}`}>({W}×{H})</span></h1>
          <p className={`${darkMode ? 'text-slate-300' : 'text-sm text-slate-600'}`}>cell size: {distribution.meta.cell_size_m} m</p>
        </div>
        <button onClick={exportJSON} className="rounded-md bg-slate-900 px-3 py-2 text-white hover:bg-slate-800">Export JSON</button>
      </div>

      <div className={`flex flex-wrap gap-2 rounded-lg border p-3 ${darkMode ? 'bg-slate-800 border-slate-700' : 'bg-white'}`}>
        {([["PATH","g","#3db86b"],["OBSTACLE","#","#9ca3af"],["RESTRICTED","X","#7b1113"],["CHARGER","C","#1e90ff"],["PACKET","P","#8b5a2b"],["EMPTY",".","#ffffff"]] as [BrushKey,string,string][])
          .map(([k,_,c])=>(
          <button key={k} onClick={()=>setBrush(k)} className={`flex items-center gap-2 rounded-md border px-3 py-1 text-sm ${brush===k ? (darkMode ? 'border-white' : 'border-slate-900') : (darkMode ? 'border-slate-600 hover:border-slate-400' : 'border-slate-300 hover:border-slate-400')}`}>
            <span className={`inline-block h-4 w-4 rounded border ${darkMode ? 'border-slate-600' : 'border-slate-300'}`} style={{background:c}}/>
            <span className={`${darkMode ? 'text-slate-100' : ''}`}>{k}</span>
          </button>
        ))}
      </div>

      <div className={`rounded-lg border p-3 overflow-auto ${darkMode ? 'bg-slate-900 border-slate-700' : 'bg-white'}`}>
        <svg width={svgW} height={svgH} className="block">
          {grid.map((row, y) =>
            row.map((ch, x) => (
              <rect key={`${x}-${y}`} x={x * CELL} y={(H - 1 - y) * CELL} width={CELL} height={CELL}
                fill={CELL_COLORS[ch] ?? "#eee"}
                stroke={darkMode ? '#334155' : '#cbd5e1'}
                strokeWidth={1}
                onMouseDown={() => { painting.current = true; paint(x, y, BRUSH_TO_CHAR[brush]); }}
                onMouseEnter={() => { if (painting.current) paint(x, y, BRUSH_TO_CHAR[brush]); }}
              />
            ))
          )}

          {grid.map((row, y) =>
            row.map((ch, x) => (
              (ch === "C" || ch === "P") ? (
                <text key={`t-${x}-${y}`} x={x * CELL + CELL * 0.5} y={(H - 1 - y) * CELL + CELL * 0.62}
                  fontSize={Math.max(10, CELL * 0.36)} textAnchor="middle"
                  fill={ch === "C" ? "#1e90ff" : (darkMode ? '#ffffff' : '#5b3b1a')}
                  style={{ pointerEvents: "none" }}
                >{ch}</text>
              ) : null
            ))
          )}
        </svg>
      </div>
    </div>
  );
}
