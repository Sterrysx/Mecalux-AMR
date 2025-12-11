# Frontend Requirements for Mecalux AMR Fleet Manager

Based on the backend architecture documentation, here's what the frontend must implement:

---

## 🎯 Core Requirements

### 1. **Real-Time Visualization** (20 Hz Updates)

The frontend must consume the file-based API and render real-time updates:

#### API Endpoints to Poll (`api/output/`)
```
robots.json   → 20 Hz (Robot positions, velocities, states)
tasks.json    → 1 Hz  (Task statuses)
map.json      → 1 Hz  (Dynamic obstacles)
```

#### Required Data Structure Handling

**robots.json:**
```typescript
interface RobotState {
  id: number;
  x: number;              // Pixel coordinates (0.1m resolution)
  y: number;
  vx: number;             // Velocity components
  vy: number;
  state: 'IDLE' | 'MOVING' | 'COMPUTING_PATH' | 'ARRIVED' | 'COLLISION_WAIT';
  goal: number;           // Current NavMesh node ID
  itinerary: number[];    // Ordered list of goal nodes
}
```

**tasks.json:**
```typescript
interface Task {
  id: number;
  sourceNode: number;     // Pickup POI
  destinationNode: number; // Dropoff POI
  status: 'PENDING' | 'ASSIGNED' | 'IN_PROGRESS' | 'COMPLETED';
  assignedRobotId?: number;
}
```

**map.json:**
```typescript
interface DynamicObstacle {
  x: number;
  y: number;
  type: 'BOX' | 'TEMPORARY_BLOCKAGE';
}
```

---

## 🗺️ Map Rendering Requirements

### Base Map Layers

1. **StaticBitMap Layer**
   - Render 1600×600 pixel grid (160m × 60m warehouse)
   - Resolution: 0.1m per pixel
   - Colors:
     - `.` (Walkable) → White/Light gray
     - `#` (Obstacle) → Dark gray
     - `X` (Restricted) → Red tint

2. **NavMesh Visualization** (Optional Debug Layer)
   - Render 0.5m × 0.5m tiles
   - Show connections between nodes
   - Highlight goal nodes in robot itineraries

3. **POI Markers**
   - **Charging Stations** (C1, C2...) → Blue icons
   - **Pickup Points** (P1-P30) → Green icons
   - **Dropoff Points** (D1-D30) → Orange icons
   - Load from `layer1/assets/poi_config.json`

### Dynamic Layers

4. **Robot Rendering**
   - Circular body (0.3m radius = 3 pixels at 0.1m/px)
   - Color-coded by state:
     - IDLE → Gray
     - MOVING → Green
     - CHARGING → Blue
     - COLLISION_WAIT → Yellow
     - ERROR → Red
   - Show velocity vector (arrow)
   - Display robot ID label
   - Render path waypoints (if available)

5. **Task Visualization**
   - Draw lines from pickup → dropoff
   - Color by status:
     - PENDING → Gray dashed
     - ASSIGNED → Yellow dashed
     - IN_PROGRESS → Green solid
     - COMPLETED → Transparent/fade out

---

## 📊 Dashboard Components

### Required Widgets

#### 1. Fleet Status Overview
```
┌─────────────────────────────────────┐
│ Total Robots: 6                     │
│ Active: 4  Idle: 1  Charging: 1     │
│ Average Battery: 82%                 │
└─────────────────────────────────────┘
```

#### 2. Task Queue Stats
```
┌─────────────────────────────────────┐
│ Pending: 12    Assigned: 8          │
│ In Progress: 6  Completed: 142      │
│ Throughput: 23 tasks/min             │
└─────────────────────────────────────┘
```

#### 3. System Performance Metrics
```
┌─────────────────────────────────────┐
│ VRP Solve Time: 124 ms              │
│ Avg Path Query: 48 ms                │
│ Physics Loop: 20 Hz (3.2 ms/tick)   │
└─────────────────────────────────────┘
```

#### 4. Scenario Indicator
```
┌─────────────────────────────────────┐
│ Current Strategy:                   │
│ 🔹 Scenario B: Cheap Insertion       │
│ Last triggered: 2s ago               │
└─────────────────────────────────────┘
```
Show which scheduling scenario is active:
- **Scenario A**: Boot-Up (Full VRP Solve)
- **Scenario B**: Streaming (≤5 tasks)
- **Scenario C**: Batch (>5 tasks with starter tasks)

#### 5. Individual Robot Cards
```
┌─────────────────────────────────────┐
│ Robot #0                  [MOVING]  │
│ Battery: ████████░░ 85%             │
│ Tasks in itinerary: 3                │
│ Current goal: P5 (Pickup Zone)      │
│ Speed: 1.2 m/s                       │
└─────────────────────────────────────┘
```

---

## 🎮 Interactive Controls

### Essential Features

1. **Task Injection UI**
   ```
   ┌─────────────────────────────────────┐
   │ Inject New Tasks                    │
   │ Number: [5]  [Random] [Custom]      │
   │ [Inject Tasks]                      │
   │                                     │
   │ ⚠ Warning: >5 tasks triggers        │
   │   Background Re-plan (Scenario C)   │
   └─────────────────────────────────────┘
   ```

2. **Robot Control Panel**
   - Send robot to charging station
   - Assign manual goal node
   - Emergency stop
   - View full itinerary

3. **Algorithm Selection** (Already implemented in TaskPlannerTest.tsx)
   ```typescript
   enum AlgorithmID {
     COMPARISON = 0,    // All 3 algorithms
     ALNS = 1,          // Best quality
     TABU = 2,          // Balanced
     SA = 3,            // Simulated Annealing
     HC = 4,            // Hill Climbing
     HEURISTIC = -1     // Greedy + Hill Climbing only
   }
   ```

4. **Playback Controls**
   - Play/Pause simulation
   - Speed control (0.5x, 1x, 2x, 5x)
   - Reset to initial state

---

## 🔄 Real-Time Communication

### Polling Strategy

```typescript
// High-frequency robot updates
setInterval(() => {
  fetch('/api/output/robots.json')
    .then(r => r.json())
    .then(data => updateRobotPositions(data.robots));
}, 50); // 20 Hz

// Lower-frequency task updates
setInterval(() => {
  fetch('/api/output/tasks.json')
    .then(r => r.json())
    .then(data => updateTaskStates(data.tasks));
}, 1000); // 1 Hz
```

### WebSocket Alternative (if backend supports)
```typescript
const ws = new WebSocket('ws://localhost:3001');
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  switch (data.type) {
    case 'robot_update': updateRobots(data.robots); break;
    case 'task_update': updateTasks(data.tasks); break;
    case 'scenario_change': showScenarioNotification(data.scenario); break;
  }
};
```

---

## 📈 Advanced Features (Optional)

### 1. Heatmap Visualization
- Show robot traffic density over time
- Identify congestion hotspots

### 2. Path Prediction
- Render future waypoints in robot itinerary
- Show Theta* paths as smooth curves

### 3. Collision Avoidance Debug View
- Visualize ORCA velocity obstacles
- Show preferred vs. safe velocity vectors

### 4. Performance Graphs
- Live chart of VRP solve times
- Battery levels over time
- Task completion rate (tasks/minute)

### 5. Event Log
```
12:34:56 [Robot 3] Completed task #142 (P5 → D12)
12:34:52 [Scenario C] Background re-plan started (15 tasks)
12:34:48 [Robot 0] Battery low, returning to C1
12:34:45 [Fleet] Injected 8 new tasks
```

---

## 🛠️ Technical Implementation Notes

### Coordinate System
- **Backend**: Top-left origin (0,0), Y-axis down
- **Canvas**: Same convention (HTML5 Canvas standard)
- **SVG**: May need transform if using SVG rendering

### Performance Optimization
1. Use **Canvas** for robot rendering (not DOM elements)
2. Only update changed regions (dirty rectangles)
3. Use **Web Workers** for data processing if needed
4. Throttle non-critical updates (task status, UI widgets)

### Responsive Design
```
Desktop (≥1280px):  Split view (map 70% | controls 30%)
Tablet (768-1279px): Stacked layout with collapsible panels
Mobile (<768px):     Single view with tab navigation
```

---

## 📦 Data Flow Architecture

```
Backend (C++)
      ↓
api/output/*.json files
      ↓
Frontend Polling Service
      ↓
State Management (Redux/Zustand)
      ↓
React Components
      ↓
Canvas/SVG Rendering
```

### Recommended State Structure
```typescript
interface FleetState {
  robots: Map<number, RobotState>;
  tasks: Map<number, Task>;
  map: {
    static: BitMap;
    dynamic: DynamicObstacle[];
    pois: POI[];
  };
  system: {
    currentScenario: 'A' | 'B' | 'C';
    vrpSolverActive: boolean;
    stats: PerformanceStats;
  };
}
```

---

## 🎨 UI/UX Requirements

### Color Scheme (Match Backend Logs)
- **Primary Blue**: #1e90ff (Charging)
- **Success Green**: #3db86b (Active/Moving)
- **Warning Yellow**: #ffa500 (Collision Wait)
- **Error Red**: #e04e4e (Error state)
- **Neutral Gray**: #9ca3af (Idle/Obstacles)

### Notifications
- Show toast when scenario changes
- Alert on robot errors (COLLISION_WAIT > 5s)
- Success message on task completion batches

### Accessibility
- ARIA labels for robot states
- Keyboard navigation for controls
- High contrast mode option

---

## 🚀 Implementation Priority

### Phase 1: Core Visualization (Week 1)
- [x] Map rendering (StaticBitMap)
- [ ] Robot position updates (20 Hz)
- [ ] POI markers
- [ ] Basic task visualization

### Phase 2: Dashboard & Controls (Week 2)
- [ ] Fleet status widget
- [ ] Task queue stats
- [ ] Task injection UI
- [ ] Playback controls

### Phase 3: Advanced Features (Week 3)
- [ ] Scenario indicator
- [ ] Robot detail panels
- [ ] Performance metrics
- [ ] Event log

### Phase 4: Polish & Optimization (Week 4)
- [ ] Responsive design
- [ ] Animation smoothing
- [ ] Performance profiling
- [ ] User testing

---

## 📚 Integration Points

### Files to Create/Modify

1. **New API Service**
   ```
   apps/fleet_manager/src/services/FleetAPI.ts
   ```
   Handles polling of `api/output/*.json` files

2. **State Management**
   ```
   apps/fleet_manager/src/stores/fleetStore.ts
   ```
   Zustand/Redux store for fleet state

3. **Canvas Renderer**
   ```
   apps/fleet_manager/src/components/WarehouseCanvas.tsx
   ```
   High-performance map + robot rendering

4. **Dashboard Widgets**
   ```
   apps/fleet_manager/src/components/
     - FleetStatusCard.tsx
     - TaskQueueWidget.tsx
     - ScenarioIndicator.tsx
     - RobotDetailPanel.tsx
   ```

5. **Control Panel**
   ```
   apps/fleet_manager/src/components/
     - TaskInjector.tsx
     - RobotController.tsx
     - PlaybackControls.tsx
   ```

---

## 🔗 Backend Integration Checklist

- [ ] Verify `api/output/` directory exists
- [ ] Confirm JSON file format matches backend output
- [ ] Test file update frequency (robots.json at 20 Hz)
- [ ] Handle missing/corrupted JSON gracefully
- [ ] Add fallback for when backend is not running
- [ ] Mock data for development/testing

---

## 📝 Example Usage Flow

1. User starts backend: `./build/fleet_manager --cli`
2. Frontend polls `api/output/robots.json` → sees 6 robots at charging stations
3. User clicks "Inject 3 Tasks" in frontend
4. Frontend calls backend API (or uses CLI interface)
5. Backend triggers **Scenario B** (Cheap Insertion)
6. Frontend sees task status change: PENDING → ASSIGNED → IN_PROGRESS
7. Frontend renders robot movement in real-time (20 Hz updates)
8. User clicks "Inject 10 Tasks"
9. Backend triggers **Scenario C** (Background Re-plan)
10. Frontend shows notification: "Background VRP solve in progress"
11. Frontend displays "Starter Tasks" being assigned immediately
12. After ~200ms, optimized tasks appended to robot itineraries

---

## ⚠️ Known Challenges

1. **File Polling Latency**
   - JSON files may not update atomically
   - Add retry logic with exponential backoff

2. **Coordinate Mapping**
   - Backend uses pixel coordinates (0.1m resolution)
   - Frontend canvas may use different scale
   - Implement proper coordinate transformation

3. **Data Synchronization**
   - robots.json updates faster than tasks.json
   - Maintain local state cache to avoid flicker

4. **Performance**
   - 20 Hz rendering with 100+ robots
   - Use RequestAnimationFrame + interpolation

---

## 📖 Reference Backend Concepts

### Three-Layer Architecture
- **Layer 1** (Mapping): Static data, loaded once
- **Layer 2** (Planning): 1 Hz decision-making
- **Layer 3** (Physics): 20 Hz execution

### Key Algorithms
- **ALNS**: Best solution quality (~100-200ms)
- **Theta***: Smooth pathfinding (~50ms avg)
- **ORCA**: Real-time collision avoidance (<5ms)

### Scheduling Scenarios
- **A (Boot-Up)**: Full VRP solve on system start
- **B (Streaming)**: Instant assignment for ≤5 tasks
- **C (Batch)**: Starter tasks + background optimize for >5 tasks

---

This document provides a complete specification for the frontend implementation based on the backend architecture. Focus on real-time visualization first, then add interactive controls and dashboard widgets progressively.
