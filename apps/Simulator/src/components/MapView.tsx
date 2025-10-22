import React from 'react';

type BrushKey = 'EMPTY' | 'PATH' | 'OBSTACLE' | 'RESTRICTED' | 'CHARGER' | 'PACKET';

export default function MapView({
  grid,
  W,
  H,
  cellColors,
  CELL,
  onDown,
  onEnter,
}: {
  grid: string[][];
  W: number;
  H: number;
  cellColors: Record<string, string>;
  CELL: number;
  onDown: (x: number, y: number) => void;
  onEnter: (x: number, y: number) => void;
}) {
  const svgW = W * CELL;
  const svgH = H * CELL;

  return (
    <div className="relative overflow-auto rounded-lg border border-slate-300 bg-white p-3">
      <svg width={svgW} height={svgH} className="block">
        {grid.map((row, y) => row.map((ch, x) => (
          <rect
            key={`${x}-${y}`}
            x={x * CELL}
            y={(H - 1 - y) * CELL}
            width={CELL}
            height={CELL}
            fill={cellColors[ch] ?? '#eee'}
            stroke="#cbd5e1"
            strokeWidth={1}
            onMouseDown={() => onDown(x, y)}
            onMouseEnter={() => onEnter(x, y)}
          />
        )))}

        {grid.map((row, y) => row.map((ch, x) => (
          (ch === 'C' || ch === 'P') ? (
            <text
              key={`t-${x}-${y}`}
              x={x * CELL + CELL * 0.5}
              y={(H - 1 - y) * CELL + CELL * 0.62}
              fontSize={Math.max(10, CELL * 0.36)}
              textAnchor="middle"
              fill={ch === 'C' ? '#1e90ff' : '#5b3b1a'}
              style={{ pointerEvents: 'none' }}
            >{ch}</text>
          ) : null
        )))}
      </svg>
    </div>
  );
}
