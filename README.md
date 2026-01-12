# FIB-PAE Project 2024-2025 Q2

## Authors

The authors of this project are:

- **Abel Perelló**
- **Adam Serrate**
- **Arnau Noguer**
- **Oriol Farres**

## What is this project?

This project was developed inside the context of PAE, a subject from FIB (Facultat d'Informàtica de Barcelona) of UPC (Universitat Politècnica de Catalunya). This subject consists in collaborations between companies and the faculty, where companies propose challenges.

Specifically, in this repository you can find our project developed in collaboration with **Mecalux**. It consists of a centralized fleet management system for Autonomous Mobile Robots (AMR) designed for logistics warehouses. The system coordinates a fleet of robots to execute pickup and dropoff tasks efficiently, avoiding collisions in real-time.

With this project we offer:

- Multi-robot coordination (1 to 100+ robots simultaneously)
- Dynamic task assignment using VRP optimization algorithms
- Dual-layer maps: NavMesh for planning and Bitmap for navigation
- Real-time collision avoidance based on the ORCA algorithm
- Reactive planning with different scheduling scenarios
- Hot task injection (add new tasks without stopping the system)
- Real-time frontend dashboard for fleet visualization

## How to execute it?

### Prerequisites

Before the execution of the app you should have installed:

- **C++17** compatible compiler (GCC 9+ or Clang 10+)
- **Node.js 18+**
- **Make**
- **POSIX threads** (pthread)

### Installation

First, you need to clone the repository and install the Node.js dependencies:

```bash
git clone <repository-url>
cd Mecalux-AMR
npm install
```

Then, you need to compile the C++ backend:

```bash
cd backend
make -j4
```

### Execution

After that, in order to execute the project you must open three terminals:

**Terminal 1 - C++ Backend (Fleet Manager):**
```bash
cd backend
./build/fleet_manager --cli
```

**Terminal 2 - API Server:**
```bash
cd api
npm start
```
The server runs at http://localhost:3001

**Terminal 3 - React Frontend:**
```bash
npm run dev:fleet
```
The application runs at http://localhost:3000

### Backend Execution Modes

The backend supports different execution modes:

```bash
# Interactive mode (stops with Enter)
./build/fleet_manager

# CLI mode (interactive task injection)
./build/fleet_manager --cli

# Batch mode (maximum speed, auto-terminates)
./build/fleet_manager --batch

# Demo mode (demonstrates all scenarios)
./build/fleet_manager --demo

# Custom options
./build/fleet_manager --tasks ../api/set_of_tasks.json --robots 8 --duration 60
```

## Development considerations

This project is a prototype developed in an academic context. For this reason, we cannot guarantee perfect functioning in all possible situations.

### Backend Technologies

- **C++17** - Main language
- **ORCA** - Collision avoidance algorithm
- **Theta*** - Any-angle pathfinding
- **ALNS/Tabu Search** - VRP optimization

### Frontend Technologies

- **React 19** - UI Framework
- **TypeScript** - Static typing
- **Zustand** - State management
- **Tailwind CSS** - Styling
- **Vite** - Build tool

### System Architecture

The system follows a three-layer architecture:

| Layer | Name | Function | Frequency |
|-------|------|----------|-----------|
| **Layer 1** | Mapping | Static infrastructure (NavMesh, POIs) | Offline |
| **Layer 2** | Planning | Task assignment (VRP solver) | 1 Hz |
| **Layer 3** | Physics | Path execution (Theta*, ORCA) | 20 Hz |

### Configuration

The system configuration can be found at `backend/system_config.json`. Task files should be placed at `api/set_of_tasks.json` following this format:

```json
{
    "format": "poi",
    "tasks": [
        {"id": 1, "pickup": "P5", "dropoff": "D12"},
        {"id": 2, "pickup": "P3", "dropoff": "D7"}
    ]
}
```

### Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| "No path found" | Start/end in obstacle | Verify POI coordinates |
| Robots stuck | ORCA deadlock | Increase `slowdownDistance` |
| Slow VRP | Too many tasks | Increase `BATCH_THRESHOLD` |
| High CPU | 20Hz too fast | Reduce `orca_tick_ms` |

For verbose debugging:

```bash
export MECALUX_DEBUG=1
./build/fleet_manager
```
