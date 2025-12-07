# Task Planner API Server

A portable Node.js backend API that interfaces with the C++ multi-robot task planner.

## 🌐 Cross-Platform Support

This server automatically detects your operating system and runs the planner accordingly:

- **Windows**: Uses WSL (Windows Subsystem for Linux) to run the planner
- **Linux**: Runs the planner directly
- **macOS**: Runs the planner directly

## 📋 Prerequisites

### All Platforms
- Node.js (v14 or higher)
- npm or yarn

### Windows-Specific
- WSL2 installed with a Linux distribution (Ubuntu recommended)
- Build tools installed in WSL: `sudo apt install build-essential`

### Linux/macOS
- GCC/G++ compiler: `sudo apt install build-essential` (Linux) or Xcode Command Line Tools (macOS)

## 🚀 Setup

1. **Install dependencies:**
   ```bash
   cd api
   npm install
   ```

2. **Compile the C++ planner** (if not already compiled):
   
   **On Windows (using WSL):**
   ```powershell
   wsl -e bash -c "cd /mnt/c/Users/YOUR_USERNAME/path/to/Mecalux-AMR/optimality/02_layer_planner && make"
   ```
   
   **On Linux/macOS:**
   ```bash
   cd ../optimality/02_layer_planner
   make
   ```

## ▶️ Running the Server

### Standard Mode
```bash
node server.js
```

The server will start on `http://localhost:3001`

### Development Mode (with auto-restart)
```bash
npm install -g nodemon
nodemon server.js
```

## 🔌 API Endpoints

### POST `/api/planner`
Run the task planner with specified parameters.

**Request Body:**
```json
{
  "algorithmId": 2,     // 0=Comparison, 1=Brute Force, 2=Greedy, 3=Hill Climbing
  "graphId": 1,         // Graph ID (1-10)
  "numTasks": 15,       // Number of tasks
  "numRobots": 3,       // Number of robots (1-10)
  "mode": "single"      // "single" or "comparison"
}
```

**Response:**
```json
{
  "algorithm": "Greedy",
  "makespan": 39.40,
  "computationTime": 0.2169,
  "robots": [
    {
      "id": 1,
      "tasks": [
        {
          "id": "T0",
          "startTime": 12.88,
          "duration": 10.25,
          "description": "Task 0"
        }
      ]
    }
  ]
}
```

### GET `/api/graphs`
Get available graph IDs.

**Response:**
```json
{
  "graphs": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
}
```

### GET `/api/health`
Health check endpoint.

**Response:**
```json
{
  "status": "ok",
  "timestamp": "2025-10-12T12:00:00.000Z"
}
```

## 🛠️ Configuration

The server uses relative paths by default, making it portable across different machines and operating systems.

### Environment Variables (Optional)

You can override the default planner directory:

```bash
export PLANNER_DIR=/path/to/your/planner
node server.js
```

**Windows:**
```powershell
$env:PLANNER_DIR="C:\path\to\your\planner"
node server.js
```

## 🔍 Troubleshooting

### Windows: "WSL not found"
- Install WSL2: `wsl --install`
- Install Ubuntu: `wsl --install -d Ubuntu`

### "Planner executable not found"
- Make sure you've compiled the planner: `make` in the `optimality/02_layer_planner` directory
- Check that `build/planner` exists in the planner directory

### Port 3001 already in use
- Kill existing processes: `Get-Process node | Stop-Process -Force` (Windows PowerShell)
- Or change the PORT in server.js: `const PORT = 3002;`

### CORS errors
- The server allows all origins by default for development
- For production, configure CORS properly in server.js

## 📦 Dependencies

- **express**: Web framework
- **cors**: Enable CORS for frontend communication
- **child_process**: Execute the C++ planner

## 🏗️ Architecture

```
Frontend (React) → API Server (Node.js) → C++ Planner (via WSL/bash)
                        ↓
                    Parse Output
                        ↓
                    JSON Response
```

The server acts as a bridge between your web interface and the C++ planner, handling:
- OS detection and appropriate command execution
- Output parsing and formatting
- Error handling and validation
- Cross-platform compatibility
