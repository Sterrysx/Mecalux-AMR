// src/pages/Isometric/IsometricView.tsx
import { useMemo, useState, useEffect } from "react";
import { useDistribution } from "../../contexts/DistributionContext";
import { useTheme } from "../../contexts/ThemeContext";

const COLORS: Record<string,string> = {
  "X":"#8b1d1e","#":"#a3a3a3","C":"#3b82f6","P":"#a16207",
  "b":"#2563eb","g":"#22c55e","r":"#ef4444",".":"#e5e7eb",
};

const TILE_SIZE = 40; // Size of each tile in 3D space
const TILE_HEIGHT = 8; // Base height of tiles

// 3D Point structure
interface Point3D {
  x: number;
  y: number;
  z: number;
}

// Project 3D point to 2D screen coordinates
function project3D(point: Point3D, orbitH = 0, orbitV = 0, distance = 800): { sx: number; sy: number; depth: number } {
  // Apply camera rotation
  const cosH = Math.cos(orbitH);
  const sinH = Math.sin(orbitH);
  const cosV = Math.cos(orbitV);
  const sinV = Math.sin(orbitV);
  
  // Rotate around Y axis (horizontal orbit)
  let rotX = point.x * cosH - point.z * sinH;
  let rotY = point.y;
  let rotZ = point.x * sinH + point.z * cosH;
  
  // Rotate around X axis (vertical orbit)
  const finalX = rotX;
  const finalY = rotY * cosV - rotZ * sinV;
  const finalZ = rotY * sinV + rotZ * cosV;
  
  // Perspective projection
  const perspectiveZ = finalZ + distance;
  const scale = distance / (perspectiveZ > 0 ? perspectiveZ : 1);
  
  return {
    sx: finalX * scale,
    sy: -finalY * scale, // Flip Y for screen coordinates
    depth: perspectiveZ
  };
}

// Get 3D height for different tile types
function getTileHeight(ch: string): number {
  switch (ch) {
    case '#': return TILE_HEIGHT * 3.5; // Obstacles are tall
    case 'X': return TILE_HEIGHT * 1.5; // Restricted areas slightly raised
    case 'C': return TILE_HEIGHT * 2.2; // Chargers are medium height
    case 'P': return TILE_HEIGHT * 2.0; // Packets are medium height
    default: return TILE_HEIGHT; // Default floor height
  }
}
function darker(hex:string, f=0.75){
  const n=(h:string)=>Math.max(0,Math.min(255,Math.round(parseInt(h,16)*f))).toString(16).padStart(2,"0");
  return `#${n(hex.slice(1,3))}${n(hex.slice(3,5))}${n(hex.slice(5,7))}`;
}

export default function IsometricView(){
  const { distribution, grid, isLoading, error } = useDistribution();
  const { darkMode } = useTheme();
  const [hover, setHover] = useState<{x:number;y:number;ch:string}|null>(null);
  const [panOffset, setPanOffset] = useState(() => {
    // Load saved pan position from localStorage
    const saved = localStorage.getItem('isometricPanOffset');
    return saved ? JSON.parse(saved) : { x: 0, y: 0 };
  });
  const [zoomLevel, setZoomLevel] = useState(() => {
    // Load saved zoom level from localStorage
    const saved = localStorage.getItem('isometricZoomLevel');
    return saved ? parseFloat(saved) : 1.0;
  });
  const [isPanning, setIsPanning] = useState(false);
  const [isOrbiting, setIsOrbiting] = useState(false);
  const [lastPanPosition, setLastPanPosition] = useState({ x: 0, y: 0 });
  const [orbitAngles, setOrbitAngles] = useState(() => {
    // Load saved orbit angles from localStorage
    const saved = localStorage.getItem('isometricOrbitAngles');
    return saved ? JSON.parse(saved) : { horizontal: Math.PI / 4, vertical: -Math.PI / 4 };
  });

  // Save pan position and zoom to localStorage whenever they change
  useEffect(() => {
    localStorage.setItem('isometricPanOffset', JSON.stringify(panOffset));
  }, [panOffset]);

  useEffect(() => {
    localStorage.setItem('isometricZoomLevel', zoomLevel.toString());
  }, [zoomLevel]);

  useEffect(() => {
    localStorage.setItem('isometricOrbitAngles', JSON.stringify(orbitAngles));
  }, [orbitAngles]);

  // Global mouse event listeners for smooth panning and orbiting
  useEffect(() => {
    const handleGlobalMouseMove = (e: MouseEvent) => {
      if (isPanning) {
        e.preventDefault();
        const deltaX = (e.clientX - lastPanPosition.x) * 0.75; //velocitat ratoli
        const deltaY = (e.clientY - lastPanPosition.y) * 0.75; //
        
        setPanOffset((prev: { x: number; y: number }) => ({
          x: prev.x + deltaX,
          y: prev.y + deltaY
        }));
        
        setLastPanPosition({ x: e.clientX, y: e.clientY });
      } else if (isOrbiting) {
        e.preventDefault();
        const deltaX = (e.clientX - lastPanPosition.x) * 0.01; // Orbit sensitivity
        const deltaY = (e.clientY - lastPanPosition.y) * 0.01;
        
        setOrbitAngles((prev: { horizontal: number; vertical: number }) => ({
          horizontal: prev.horizontal + deltaX,
          vertical: Math.max(-Math.PI/3, Math.min(Math.PI/3, prev.vertical + deltaY)) // Limit vertical orbit
        }));
        
        setLastPanPosition({ x: e.clientX, y: e.clientY });
      }
    };

    const handleGlobalMouseUp = () => {
      setIsPanning(false);
      setIsOrbiting(false);
    };

    if (isPanning || isOrbiting) {
      document.addEventListener('mousemove', handleGlobalMouseMove);
      document.addEventListener('mouseup', handleGlobalMouseUp);
    }

    return () => {
      document.removeEventListener('mousemove', handleGlobalMouseMove);
      document.removeEventListener('mouseup', handleGlobalMouseUp);
    };
  }, [isPanning, isOrbiting, lastPanPosition]);

  // Generate 3D tiles with proper depth sorting
  const tiles3D = useMemo(() => {
    if (!distribution || !grid.length) return [];
    const W = distribution.meta.width;
    const H = distribution.meta.height;
    const list: Array<{
      x: number;
      y: number;
      ch: string;
      faces: Array<{
        points: Point3D[];
        color: string;
        type: 'top' | 'side' | 'front';
      }>;
      center: Point3D;
      projectedDepth: number;
    }> = [];
    
    // Generate 3D tiles
    for (let y = 0; y < H; y++) {
      for (let x = 0; x < W; x++) {
        const ch = grid[y] && grid[y][x] ? grid[y][x] : ".";
        const height = getTileHeight(ch);
        
        // 3D coordinates (center grid at origin)
        const x3D = (x - W/2) * TILE_SIZE;
        const z3D = (y - H/2) * TILE_SIZE; // Z is the "depth" axis
        const y3D = height / 2; // Y is "up" axis
        
        const base = COLORS[ch] ?? COLORS["."];
        const side = darker(base, 0.82);
        const dark = darker(base, 0.64);
        
        // Define the 8 corners of the cube
        const corners = {
          bottomFrontLeft: { x: x3D - TILE_SIZE/2, y: 0, z: z3D + TILE_SIZE/2 },
          bottomFrontRight: { x: x3D + TILE_SIZE/2, y: 0, z: z3D + TILE_SIZE/2 },
          bottomBackLeft: { x: x3D - TILE_SIZE/2, y: 0, z: z3D - TILE_SIZE/2 },
          bottomBackRight: { x: x3D + TILE_SIZE/2, y: 0, z: z3D - TILE_SIZE/2 },
          topFrontLeft: { x: x3D - TILE_SIZE/2, y: height, z: z3D + TILE_SIZE/2 },
          topFrontRight: { x: x3D + TILE_SIZE/2, y: height, z: z3D + TILE_SIZE/2 },
          topBackLeft: { x: x3D - TILE_SIZE/2, y: height, z: z3D - TILE_SIZE/2 },
          topBackRight: { x: x3D + TILE_SIZE/2, y: height, z: z3D - TILE_SIZE/2 },
        };
        
        const faces = [
          // Top face (counter-clockwise winding for proper visibility)
          {
            points: [corners.topFrontLeft, corners.topFrontRight, corners.topBackRight, corners.topBackLeft],
            color: base,
            type: 'top' as const
          },
          // Front face
          {
            points: [corners.bottomFrontLeft, corners.bottomFrontRight, corners.topFrontRight, corners.topFrontLeft],
            color: side,
            type: 'front' as const
          },
          // Right face
          {
            points: [corners.bottomFrontRight, corners.bottomBackRight, corners.topBackRight, corners.topFrontRight],
            color: dark,
            type: 'side' as const
          },
          // Left face
          {
            points: [corners.bottomBackLeft, corners.bottomFrontLeft, corners.topFrontLeft, corners.topBackLeft],
            color: side,
            type: 'side' as const
          },
          // Back face
          {
            points: [corners.bottomBackRight, corners.bottomBackLeft, corners.topBackLeft, corners.topBackRight],
            color: dark,
            type: 'side' as const
          }
        ];
        
        const center = { x: x3D, y: y3D, z: z3D };
        const projected = project3D(center, orbitAngles.horizontal, orbitAngles.vertical);
        
        list.push({
          x, y, ch, faces, center,
          projectedDepth: projected.depth
        });
      }
    }
    
    // Sort by depth (furthest first for proper rendering)
    return list.sort((a, b) => b.projectedDepth - a.projectedDepth);
  }, [distribution, grid, orbitAngles]);

  // Now it's safe to exit early.
  // Mouse event handlers for panning, orbiting and zooming
  const handleMouseDown = (e: React.MouseEvent) => {
    if (e.button === 0) { // Left mouse button - orbit
      e.preventDefault();
      setIsOrbiting(true);
      setLastPanPosition({ x: e.clientX, y: e.clientY });
    } else if (e.button === 2) { // Right mouse button - pan
      e.preventDefault();
      setIsPanning(true);
      setLastPanPosition({ x: e.clientX, y: e.clientY });
    }
  };

  const handleWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const zoomFactor = 0.1;
    const deltaZoom = e.deltaY > 0 ? -zoomFactor : zoomFactor;
    
    setZoomLevel(prevZoom => {
      const newZoom = Math.max(0.3, Math.min(3.0, prevZoom + deltaZoom)); // Limit zoom between 0.3x and 3x
      return newZoom;
    });
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault(); // Prevent right-click context menu
  };

  if (error) return <div className="p-6 text-red-600">Error: {error}</div>;
  if (isLoading || !distribution) return <div className="p-6">Loading…</div>;

  const W = distribution.meta.width;
  const H = distribution.meta.height;

  // Calculate 3D viewport dimensions
  const baseMapW = W * TILE_SIZE * 2; // Wider for 3D view
  const baseMapH = H * TILE_SIZE * 2; // Taller for 3D view
  const mapW = baseMapW / zoomLevel; // Apply zoom
  const mapH = baseMapH / zoomLevel; // Apply zoom
  
  // Calculate the center of the 3D grid
  const gridCenter3D = { x: 0, y: TILE_HEIGHT, z: 0 }; // Already centered at origin
  const {sx: centerX, sy: centerY} = project3D(gridCenter3D, orbitAngles.horizontal, orbitAngles.vertical);
  
  // Position viewBox so the grid center appears in the middle of the viewport
  const vbX = centerX - mapW/2 - panOffset.x / zoomLevel;
  const vbY = centerY - mapH/2 - panOffset.y / zoomLevel;

  return (
    <div className={`mx-auto max-w-[1100px] p-6 space-y-4 ${darkMode ? 'text-slate-100' : 'text-slate-900'}`}>
      <div className="flex items-end justify-between">
        <h1 className={`text-2xl font-semibold ${darkMode ? 'text-white' : ''}`}>
          {distribution.meta.name} <span className={`${darkMode ? 'text-slate-300' : 'text-slate-500'}`}>(isometric)</span>
        </h1>
        <div className="flex gap-4 items-center">
          {hover ? (
            <div className={`${darkMode ? 'text-sm rounded-md bg-slate-800 text-white px-2 py-1' : 'text-sm rounded-md bg-slate-900 text-white px-2 py-1'}`}>
              {hover.ch} (x:{hover.x}, y:{hover.y})
            </div>
          ) : (
            <div className={`text-sm px-2 py-1 ${darkMode ? 'text-slate-300' : 'text-slate-500'}`}>
              Left-click: Orbit • Right-click: Pan • Scroll: Zoom
            </div>
          )}
          
          <div className="flex items-center gap-2">
            <span className={`${darkMode ? 'text-slate-300 text-sm' : 'text-sm text-slate-600'}`}>
              Zoom: {Math.round(zoomLevel * 100)}%
            </span>
            <button 
              onClick={() => setZoomLevel(prev => Math.max(0.3, prev - 0.2))}
              className={`${darkMode ? 'text-sm rounded-md bg-slate-700 hover:bg-slate-600 text-slate-100 px-2 py-1' : 'text-sm rounded-md bg-slate-200 hover:bg-slate-300 text-slate-700 px-2 py-1'}`}
            >
              -
            </button>
            <button 
              onClick={() => setZoomLevel(prev => Math.min(3.0, prev + 0.2))}
              className={`${darkMode ? 'text-sm rounded-md bg-slate-700 hover:bg-slate-600 text-slate-100 px-2 py-1' : 'text-sm rounded-md bg-slate-200 hover:bg-slate-300 text-slate-700 px-2 py-1'}`}
            >
              +
            </button>
          </div>
          
          <button 
            onClick={() => {
              // Reset to default 45-degree isometric view
              setOrbitAngles({ horizontal: Math.PI / 4, vertical: -Math.PI / 4 });
              setPanOffset({ x: 0, y: 0 });
              setZoomLevel(1.0);
            }}
            className={`${darkMode ? 'text-sm rounded-md bg-slate-700 hover:bg-slate-600 text-slate-100 px-2 py-1' : 'text-sm rounded-md bg-slate-200 hover:bg-slate-300 text-slate-700 px-2 py-1'}`}
          >
            Reset View
          </button>
        </div>
      </div>

      <div 
        className={`${darkMode ? 'relative overflow-auto rounded-lg border border-slate-700 bg-slate-900 p-6' : 'relative overflow-auto rounded-lg border border-slate-300 bg-gradient-to-br from-slate-100 to-slate-200 p-6'}`}
        onMouseDown={handleMouseDown}
        onWheel={handleWheel}
        onContextMenu={handleContextMenu}
        style={{ cursor: isPanning ? 'grabbing' : isOrbiting ? 'move' : 'grab' }}
      >
        <svg
          viewBox={`${vbX} ${vbY} ${mapW} ${mapH}`}
          width="100%" height="520"
          preserveAspectRatio="xMidYMid meet"
        >
          <g transform={`translate(0, 24)`}>
            {tiles3D.map(({x, y, ch, faces}) => {
              return (
                <g key={`${x}-${y}`} onMouseEnter={() => setHover({x, y, ch})} onMouseLeave={() => setHover(null)}>
                      {faces.map((face, faceIndex) => {
                    // Project each point of the face to 2D
                    const projectedPoints = face.points.map(point => 
                      project3D(point, orbitAngles.horizontal, orbitAngles.vertical)
                    );
                    
                    // Create SVG polygon points string
                    const pointsString = projectedPoints
                      .map(p => `${p.sx},${p.sy}`)
                      .join(' ');
                    
                    // Calculate face normal for visibility culling (simple backface culling)
                    const p1 = projectedPoints[0];
                    const p2 = projectedPoints[1];
                    const p3 = projectedPoints[2];
                    
                    const v1x = p2.sx - p1.sx;
                    const v1y = p2.sy - p1.sy;
                    const v2x = p3.sx - p1.sx;
                    const v2y = p3.sy - p1.sy;
                    
                    const cross = v1x * v2y - v1y * v2x;
                    
                    // Always render top faces, and render other faces based on normal
                      if (face.type === 'top' || cross > 0) {
                      return (
                        <polygon
                          key={`face-${faceIndex}`}
                          points={pointsString}
                          fill={face.color}
                          stroke={darkMode ? '#111827' : '#666'}
                          strokeWidth={0.5}
                          style={{ opacity: face.type === 'top' ? 1 : 0.9 }}
                        />
                      );
                    }
                    return null;
                  })}
                  
                  {/* Add text labels for special tiles */}
                      {(ch === "C" || ch === "P") && (() => {
                    const centerPoint = project3D(
                      { x: (x - W/2) * TILE_SIZE, y: getTileHeight(ch), z: (y - H/2) * TILE_SIZE },
                      orbitAngles.horizontal, 
                      orbitAngles.vertical
                    );
                    return (
                      <text 
                        x={centerPoint.sx} 
                        y={centerPoint.sy - 10} 
                        textAnchor="middle" 
                        fontSize={14} 
                        fill={darkMode ? '#ffffff' : '#000'} 
                        fontWeight={700}
                        style={{ filter: 'drop-shadow(1px 1px 1px rgba(0,0,0,0.5))' }}
                      >
                        {ch}
                      </text>
                    );
                  })()}
                </g>
              );
            })}
          </g>
        </svg>
      </div>

      <div className="flex flex-wrap gap-3">
        {Object.entries({"Restricted":"X","Obstacle":"#","Charger":"C","Packet":"P","Path":"g","Empty":"."}).map(([label,ch])=>(
          <div key={label} className="flex items-center gap-2">
            <span className={`inline-block h-4 w-4 rounded border ${darkMode ? 'border-slate-600' : 'border-slate-300'}`} style={{background: COLORS[ch]}}/>
            <span className={`text-sm ${darkMode ? 'text-slate-300' : ''}`}>{label}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
