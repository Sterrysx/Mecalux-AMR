// High-performance Canvas renderer for warehouse map, robots, tasks, and POIs
import { useEffect, useRef, useState } from 'react';
import { useFleetStore } from '../stores/fleetStore';
import { RobotState, POI, Task } from '../services/FleetAPI';

interface WarehouseCanvasProps {
  width?: number;
  height?: number;
  className?: string;
}

// Map configuration (from backend specs)
const MAP_WIDTH_PIXELS = 1600;   // 160m warehouse
const MAP_HEIGHT_PIXELS = 600;   // 60m warehouse

// Visual constants
const ROBOT_RADIUS = 3;          // 0.3m radius = 3 pixels
const POI_RADIUS = 5;
const VELOCITY_ARROW_SCALE = 2;

// Colors
const COLORS = {
  background: '#f8fafc',
  grid: '#e2e8f0',
  walkable: '#ffffff',
  obstacle: '#9ca3af',
  restricted: '#7b1113',
  
  // Robot states
  robotIdle: '#94a3b8',
  robotMoving: '#3db86b',
  robotCarrying: '#2b6ef3',
  robotCharging: '#1e90ff',
  robotCollisionWait: '#ffa500',
  robotError: '#e04e4e',
  
  // POI types
  poiCharging: '#1e90ff',
  poiPickup: '#3db86b',
  poiDropoff: '#e04e4e',
  
  // Tasks
  taskPending: '#cbd5e1',
  taskAssigned: '#fbbf24',
  taskInProgress: '#3db86b',
  taskCompleted: '#10b981',
  
  // UI
  text: '#1e293b',
  textLight: '#64748b'
};

export default function WarehouseCanvas({ width = 1200, height = 600, className = '' }: WarehouseCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [scale, setScale] = useState(1);
  const [offset, setOffset] = useState({ x: 0, y: 0 });
  const [isDragging, setIsDragging] = useState(false);
  const [dragStart, setDragStart] = useState({ x: 0, y: 0 });
  
  // Get data from store
  const robots = useFleetStore(state => Array.from(state.robots.values()));
  const tasks = useFleetStore(state => Array.from(state.tasks.values()));
  const pois = useFleetStore(state => state.pois);
  const dynamicObstacles = useFleetStore(state => state.dynamicObstacles);

  // Calculate scale to fit map
  useEffect(() => {
    const scaleX = width / MAP_WIDTH_PIXELS;
    const scaleY = height / MAP_HEIGHT_PIXELS;
    setScale(Math.min(scaleX, scaleY) * 0.95); // 95% to leave margin
  }, [width, height]);

  // Render loop
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animationFrameId: number;

    const render = () => {
      // Clear canvas
      ctx.fillStyle = COLORS.background;
      ctx.fillRect(0, 0, width, height);

      // Apply transform
      ctx.save();
      ctx.translate(offset.x, offset.y);
      ctx.scale(scale, scale);

      // Draw grid
      drawGrid(ctx);
      
      // Draw POIs
      drawPOIs(ctx, pois);
      
      // Draw tasks
      drawTasks(ctx, tasks, pois);
      
      // Draw dynamic obstacles
      drawObstacles(ctx, dynamicObstacles);
      
      // Draw robots
      drawRobots(ctx, robots);

      ctx.restore();

      animationFrameId = requestAnimationFrame(render);
    };

    render();

    return () => {
      cancelAnimationFrame(animationFrameId);
    };
  }, [robots, tasks, pois, dynamicObstacles, scale, offset, width, height]);

  // Drawing functions
  const drawGrid = (ctx: CanvasRenderingContext2D) => {
    ctx.strokeStyle = COLORS.grid;
    ctx.lineWidth = 0.5;

    // Vertical lines every 50 pixels (5m)
    for (let x = 0; x <= MAP_WIDTH_PIXELS; x += 50) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, MAP_HEIGHT_PIXELS);
      ctx.stroke();
    }

    // Horizontal lines every 50 pixels (5m)
    for (let y = 0; y <= MAP_HEIGHT_PIXELS; y += 50) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(MAP_WIDTH_PIXELS, y);
      ctx.stroke();
    }
  };

  const drawPOIs = (ctx: CanvasRenderingContext2D, pois: POI[]) => {
    pois.forEach(poi => {
      const color = poi.type === 'CHARGING' ? COLORS.poiCharging :
                    poi.type === 'PICKUP' ? COLORS.poiPickup :
                    COLORS.poiDropoff;

      // Draw circle
      ctx.fillStyle = color;
      ctx.beginPath();
      ctx.arc(poi.x, poi.y, POI_RADIUS, 0, Math.PI * 2);
      ctx.fill();

      // Draw label
      ctx.fillStyle = COLORS.text;
      ctx.font = '8px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(poi.id, poi.x, poi.y - POI_RADIUS - 2);
    });
  };

  const drawTasks = (ctx: CanvasRenderingContext2D, tasks: Task[], pois: POI[]) => {
    tasks.forEach(task => {
      // Find source and destination POIs
      const sourcePOI = pois.find(p => p.nodeId === task.sourceNode);
      const destPOI = pois.find(p => p.nodeId === task.destinationNode);

      if (!sourcePOI || !destPOI) return;

      // Color based on status
      const color = task.status === 'PENDING' ? COLORS.taskPending :
                    task.status === 'ASSIGNED' ? COLORS.taskAssigned :
                    task.status === 'IN_PROGRESS' ? COLORS.taskInProgress :
                    COLORS.taskCompleted;

      // Draw line
      ctx.strokeStyle = color;
      ctx.lineWidth = task.status === 'IN_PROGRESS' ? 2 : 1;
      ctx.setLineDash(task.status === 'PENDING' || task.status === 'ASSIGNED' ? [5, 5] : []);
      ctx.globalAlpha = task.status === 'COMPLETED' ? 0.3 : 0.6;

      ctx.beginPath();
      ctx.moveTo(sourcePOI.x, sourcePOI.y);
      ctx.lineTo(destPOI.x, destPOI.y);
      ctx.stroke();

      ctx.globalAlpha = 1;
      ctx.setLineDash([]);
    });
  };

  const drawObstacles = (ctx: CanvasRenderingContext2D, obstacles: any[]) => {
    obstacles.forEach(obstacle => {
      ctx.fillStyle = '#ef4444';
      ctx.fillRect(obstacle.x - 2, obstacle.y - 2, 4, 4);
    });
  };

  const drawRobots = (ctx: CanvasRenderingContext2D, robots: RobotState[]) => {
    robots.forEach(robot => {
      // Color based on state
      const color = robot.state === 'IDLE' ? COLORS.robotIdle :
                    robot.state === 'MOVING' ? COLORS.robotMoving :
                    robot.state === 'CARRYING' ? COLORS.robotCarrying :
                    robot.state === 'COLLISION_WAIT' ? COLORS.robotCollisionWait :
                    COLORS.robotError;

      // Draw robot body
      ctx.fillStyle = color;
      ctx.beginPath();
      ctx.arc(robot.x, robot.y, ROBOT_RADIUS, 0, Math.PI * 2);
      ctx.fill();

      // Draw outline
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 1;
      ctx.stroke();

      // Draw velocity arrow
      if (robot.state === 'MOVING' && (robot.vx !== 0 || robot.vy !== 0)) {
        const arrowLength = Math.sqrt(robot.vx * robot.vx + robot.vy * robot.vy) * VELOCITY_ARROW_SCALE;
        const angle = Math.atan2(robot.vy, robot.vx);

        ctx.strokeStyle = color;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(robot.x, robot.y);
        ctx.lineTo(
          robot.x + Math.cos(angle) * arrowLength,
          robot.y + Math.sin(angle) * arrowLength
        );
        ctx.stroke();

        // Arrow head
        const headSize = 3;
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.moveTo(
          robot.x + Math.cos(angle) * arrowLength,
          robot.y + Math.sin(angle) * arrowLength
        );
        ctx.lineTo(
          robot.x + Math.cos(angle - 2.5) * (arrowLength - headSize),
          robot.y + Math.sin(angle - 2.5) * (arrowLength - headSize)
        );
        ctx.lineTo(
          robot.x + Math.cos(angle + 2.5) * (arrowLength - headSize),
          robot.y + Math.sin(angle + 2.5) * (arrowLength - headSize)
        );
        ctx.closePath();
        ctx.fill();
      }

      // Draw robot ID
      ctx.fillStyle = COLORS.text;
      ctx.font = 'bold 10px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(`#${robot.id}`, robot.x, robot.y - ROBOT_RADIUS - 5);
    });
  };

  // Mouse handlers for pan/zoom
  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true);
    setDragStart({ x: e.clientX - offset.x, y: e.clientY - offset.y });
  };

  const handleMouseMove = (e: React.MouseEvent) => {
    if (!isDragging) return;
    setOffset({
      x: e.clientX - dragStart.x,
      y: e.clientY - dragStart.y
    });
  };

  const handleMouseUp = () => {
    setIsDragging(false);
  };

  const handleWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const delta = e.deltaY > 0 ? 0.9 : 1.1;
    setScale(prev => Math.max(0.1, Math.min(5, prev * delta)));
  };

  return (
    <div className={`relative ${className}`}>
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        className="border border-gray-200 rounded-lg cursor-move"
        onMouseDown={handleMouseDown}
        onMouseMove={handleMouseMove}
        onMouseUp={handleMouseUp}
        onMouseLeave={handleMouseUp}
        onWheel={handleWheel}
      />
      
      {/* Legend */}
      <div className="absolute top-4 right-4 bg-white/90 backdrop-blur-sm p-3 rounded-lg shadow-lg text-xs">
        <div className="font-semibold mb-2">Legend</div>
        <div className="space-y-1">
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full" style={{ backgroundColor: COLORS.robotMoving }}></div>
            <span>Moving</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full" style={{ backgroundColor: COLORS.robotIdle }}></div>
            <span>Idle</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full" style={{ backgroundColor: COLORS.robotCharging }}></div>
            <span>Charging</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full" style={{ backgroundColor: COLORS.poiPickup }}></div>
            <span>Pickup</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded-full" style={{ backgroundColor: COLORS.poiDropoff }}></div>
            <span>Dropoff</span>
          </div>
        </div>
        <div className="mt-2 pt-2 border-t border-gray-200 text-gray-500">
          Scroll to zoom • Drag to pan
        </div>
      </div>
      
      {/* FPS counter */}
      <div className="absolute bottom-4 right-4 bg-black/70 text-white px-2 py-1 rounded text-xs font-mono">
        {robots.length} robots • {tasks.length} tasks
      </div>
    </div>
  );
}
