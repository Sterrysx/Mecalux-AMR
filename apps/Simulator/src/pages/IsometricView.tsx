// src/pages/IsometricView.tsx
import { useEffect, useMemo, useState } from "react";

type Dist = {
  meta: { name: string; width: number; height: number; cell_size_m: number };
  ascii_map: string[]; // top-first
};

const COLORS: Record<string,string> = {
  "X":"#8b1d1e","#":"#a3a3a3","C":"#3b82f6","P":"#a16207",
  "b":"#2563eb","g":"#22c55e","r":"#ef4444",".":"#e5e7eb",
};

const TILE_W = 64;
const TILE_H = 32;

function isoProject(x:number, y:number) {
  return { sx: (x - y) * (TILE_W/2), sy: (x + y) * (TILE_H/2) };
}
function darker(hex:string, f=0.75){
  const n=(h:string)=>Math.max(0,Math.min(255,Math.round(parseInt(h,16)*f))).toString(16).padStart(2,"0");
  return `#${n(hex.slice(1,3))}${n(hex.slice(3,5))}${n(hex.slice(5,7))}`;
}

export default function IsometricView(){
  const [dist, setDist] = useState<Dist | null>(null);
  const [err, setErr] = useState<string | null>(null);
  const [hover, setHover] = useState<{x:number;y:number;ch:string}|null>(null);

  useEffect(()=>{
    fetch("/distributions/Distribution1.json")
      .then(r=>r.json())
      .then((d:Dist)=>{
        d.ascii_map = d.ascii_map.map(r=>r.replace(/\s+/g,""));
        setDist(d);
      })
      .catch(e=>setErr(String(e)));
  },[]);

  // ✅ Always call hooks before any early return.
  const tiles = useMemo(() => {
    if (!dist) return [];
    const W = dist.meta.width;
    const H = dist.meta.height;
    const list:{x:number;y:number;ch:string}[] = [];
    for (let r=0; r<dist.ascii_map.length; r++){
      const row = dist.ascii_map[r];
      const y = (H - 1) - r;
      for (let x=0; x<W; x++) list.push({x,y,ch: row[x] ?? "."});
    }
    return list.sort((a,b)=>(a.x+a.y)-(b.x+b.y));
  }, [dist]);

  // Now it's safe to exit early.
  if (err)  return <div className="p-6 text-red-600">Error: {err}</div>;
  if (!dist) return <div className="p-6">Loading…</div>;

  const W = dist.meta.width, H = dist.meta.height;

  // viewBox keeps things centered & visible regardless of container size
  const mapW  = (W+H) * (TILE_W/2);
  const mapH  = (W+H) * (TILE_H/2) + 64;
  const vbX   = -mapW/2;
  const vbY   = 0;

  return (
    <div className="mx-auto max-w-[1100px] p-6 space-y-4">
      <div className="flex items-end justify-between">
        <h1 className="text-2xl font-semibold">
          {dist.meta.name} <span className="text-slate-500">(isometric)</span>
        </h1>
        {hover ? (
          <div className="text-sm rounded-md bg-slate-900 text-white px-2 py-1">
            {hover.ch} (x:{hover.x}, y:{hover.y})
          </div>
        ) : <div />}
      </div>

      <div className="relative overflow-auto rounded-lg border border-slate-300 bg-gradient-to-br from-slate-100 to-slate-200 p-6">
        <svg
          viewBox={`${vbX} ${vbY} ${mapW} ${mapH}`}
          width="100%" height="520"
          preserveAspectRatio="xMidYMid meet"
        >
          <g transform={`translate(0, 24)`}>
            {tiles.map(({x,y,ch})=>{
              const {sx, sy} = isoProject(x,y);
              const cx = sx, cy = sy;
              const base = COLORS[ch] ?? COLORS["."];
              const side = darker(base, 0.82);
              const dark = darker(base, 0.64);

              const top    = `${cx},${cy - TILE_H/2}`;
              const right  = `${cx + TILE_W/2},${cy}`;
              const bottom = `${cx},${cy + TILE_H/2}`;
              const left   = `${cx - TILE_W/2},${cy}`;

              const HPIX = (ch==="#"?28 : ch==="X"?14 : ch==="C"?18 : ch==="P"?20 : 0);
              const tY = cy - HPIX;

              const top2    = `${cx},${tY - TILE_H/2}`;
              const right2  = `${cx + TILE_W/2},${tY}`;
              const left2   = `${cx - TILE_W/2},${tY}`;

              return (
                <g key={`${x}-${y}`} onMouseEnter={()=>setHover({x,y,ch})} onMouseLeave={()=>setHover(null)}>
                  <polygon points={`${top} ${right} ${bottom} ${left}`} fill={base} stroke="#9aa1aa" strokeWidth={0.5} />
                  {HPIX>0 && (
                    <>
                      <polygon points={`${top2} ${right2} ${cx},${tY + TILE_H/2} ${left2}`} fill={base}/>
                      <polygon points={`${right2} ${right} ${bottom} ${cx},${tY + TILE_H/2}`} fill={side}/>
                      <polygon points={`${left2} ${cx},${tY + TILE_H/2} ${bottom} ${left}`} fill={dark}/>
                    </>
                  )}
                  {(ch==="C" || ch==="P") && (
                    <text x={cx} y={tY - 6} textAnchor="middle" fontSize={12} fill="#fff" fontWeight={700}>
                      {ch}
                    </text>
                  )}
                </g>
              );
            })}
          </g>
        </svg>
      </div>

      <div className="flex flex-wrap gap-3">
        {Object.entries({"Restricted":"X","Obstacle":"#","Charger":"C","Packet":"P","Path":"g","Empty":"."}).map(([label,ch])=>(
          <div key={label} className="flex items-center gap-2">
            <span className="inline-block h-4 w-4 rounded border border-slate-300" style={{background: COLORS[ch]}}/>
            <span className="text-sm">{label}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
