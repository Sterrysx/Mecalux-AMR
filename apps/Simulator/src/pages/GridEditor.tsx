import { useEffect, useRef, useState } from "react";

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

type BrushKey = "EMPTY" | "PATH" | "OBSTACLE" | "RESTRICTED" | "CHARGER" | "PACKET";
const BRUSH_TO_CHAR: Record<BrushKey, string> = {
  EMPTY: ".", PATH: "g", OBSTACLE: "#", RESTRICTED: "X", CHARGER: "C", PACKET: "P",
};

const CELL_COLORS: Record<string,string> = {
  "X":"#7b1113","#":"#9ca3af","C":"#1e90ff","P":"#8b5a2b","b":"#2b6ef3","g":"#3db86b","r":"#e04e4e",".":"#ffffff",
};

export default function GridEditor() {
  const [data, setData] = useState<DistFile | null>(null);
  const [grid, setGrid] = useState<string[][]>([]);
  const [brush, setBrush] = useState<BrushKey>("PATH");
  const [err, setErr] = useState<string | null>(null);
  const painting = useRef(false);

  useEffect(() => {
    fetch("/distributions/Distribution1.json")
      .then(r => r.json())
      .then((d: DistFile) => {
        const W = d.meta.width, H = d.meta.height;
        const rows = d.ascii_map.map(r => r.replace(/\s+/g,""));
        const g: string[][] = Array.from({length: H}, () => Array(W).fill("."));
        for (let r = 0; r < rows.length; r++) {
          const y = (H - 1) - r;
          for (let x = 0; x < Math.min(W, rows[r].length); x++) g[y][x] = rows[r][x];
        }
        setData(d); setGrid(g);
      })
      .catch(e => setErr(String(e)));
  }, []);

  const W = data?.meta.width ?? 0, H = data?.meta.height ?? 0;

  const paint = (x:number,y:number,val:string)=>{
    setGrid(prev => { const n = prev.map(r=>r.slice()); n[y][x]=val; return n; });
  };

  const exportJSON = ()=>{
    if (!data) return;
    const rowsTop:string[]=[]; for (let r=H-1;r>=0;r--) rowsTop.push(grid[r].join(""));
    const chargers:Charger[]=[], packets:Packet[]=[]; let ci=1,pi=1;
    for (let y=0;y<H;y++) for (let x=0;x<W;x++){
      const ch=grid[y][x]; if(ch==="C") chargers.push({id:`C${ci++}`,x,y,capacity:1});
      if(ch==="P") packets.push({id:`P${pi++}`,x,y,weight:1});
    }
    const out:DistFile={ meta: data.meta, legend: data.legend, ascii_map: rowsTop, objects:{chargers,packets}, notes: data.notes };
    const blob = new Blob([JSON.stringify(out,null,2)],{type:"application/json"});
    const a=document.createElement("a"); a.href=URL.createObjectURL(blob); a.download=`${data.meta.name||"distribution"}-edited.json`; a.click(); URL.revokeObjectURL(a.href);
  };

  const CELL=40, svgW=W*CELL, svgH=H*CELL;
  if (err) return <div className="p-6 text-red-600">Error: {err}</div>;
  if (!data) return <div className="p-6">Loading…</div>;

  return (
    <div className="mx-auto max-w-[1100px] p-6 space-y-4" onMouseUp={()=>painting.current=false} onMouseLeave={()=>painting.current=false}>
      <div className="flex items-end justify-between">
        <div>
          <h1 className="text-2xl font-semibold">{data.meta.name} <span className="text-slate-500">({W}×{H})</span></h1>
          <p className="text-sm text-slate-600">cell size: {data.meta.cell_size_m} m</p>
        </div>
        <button onClick={exportJSON} className="rounded-md bg-slate-900 px-3 py-2 text-white hover:bg-slate-800">Export JSON</button>
      </div>

      <div className="flex flex-wrap gap-2 rounded-lg border p-3 bg-white">
        {([["PATH","g","#3db86b"],["OBSTACLE","#","#9ca3af"],["RESTRICTED","X","#7b1113"],["CHARGER","C","#1e90ff"],["PACKET","P","#8b5a2b"],["EMPTY",".","#ffffff"]] as [BrushKey,string,string][])
          .map(([k,_,c])=>(
          <button key={k} onClick={()=>setBrush(k)} className={`flex items-center gap-2 rounded-md border px-3 py-1 text-sm ${brush===k?"border-slate-900":"border-slate-300 hover:border-slate-400"}`}>
            <span className="inline-block h-4 w-4 rounded border border-slate-300" style={{background:c}}/>
            <span>{k}</span>
          </button>
        ))}
      </div>

      <div className="rounded-lg border bg-white p-3 overflow-auto">
        <svg width={svgW} height={svgH} className="block">
          {grid.map((row,y)=>row.map((ch,x)=>(
            <rect key={`${x}-${y}`} x={x*CELL} y={(H-1-y)*CELL} width={CELL} height={CELL}
              fill={CELL_COLORS[ch]??"#eee"} stroke="#cbd5e1" strokeWidth={1}
              onMouseDown={()=>{painting.current=true;paint(x,y,BRUSH_TO_CHAR[brush]);}}
              onMouseEnter={()=>{ if(painting.current) paint(x,y,BRUSH_TO_CHAR[brush]); }}
            />
          )))}
          {grid.map((row,y)=>row.map((ch,x)=>(
            (ch==="C"||ch==="P")?(
              <text key={`t-${x}-${y}`} x={x*CELL + CELL*0.5} y={(H-1-y)*CELL + CELL*0.62}
                fontSize={Math.max(10,CELL*0.36)} textAnchor="middle" fill={ch==="C"?"#1e90ff":"#5b3b1a"} style={{pointerEvents:"none"}}>{ch}</text>
            ):null
          )))}
        </svg>
      </div>
    </div>
  );
}
