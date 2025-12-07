# Real-Time Simulator - Quick Setup Guide

## Installation Commands

Run these commands in PowerShell to set up everything:

### 1. Install Backend Dependencies
```powershell
cd C:\Users\Adam\Desktop\PAE\Mecalux-AMR\api
npm install
```

This will install:
- `express` - Web server framework
- `cors` - Cross-origin resource sharing
- `ws` - WebSocket library for real-time communication

### 2. Install Frontend Dependencies
```powershell
cd C:\Users\Adam\Desktop\PAE\Mecalux-AMR\apps\Simulator
npm install
```

### 3. Build the Real-Time Simulator (if not already built)
```powershell
wsl
cd /mnt/c/Users/Adam/Desktop/PAE/Mecalux-AMR/optimality/real_time_simulator
make
exit
```

## Running the Application

### Start Backend Server
```powershell
cd C:\Users\Adam\Desktop\PAE\Mecalux-AMR\api
node server.js
```

Keep this terminal open. You should see:
```
Server running on http://0.0.0.0:3001
```

### Start Frontend (in a new terminal)
```powershell
cd C:\Users\Adam\Desktop\PAE\Mecalux-AMR\apps\Simulator
npm run dev
```

You should see:
```
VITE ready in XXXms
Local: http://localhost:5173/
```

### Access the Application

Open your browser to: **http://localhost:5173/realtime**

## Features Implemented

✅ **WebSocket Communication**: Real-time bidirectional connection between frontend and backend
✅ **Simulator Control**: Start/Stop the C++ real-time simulator from the web interface
✅ **Live Output Console**: See simulator output in real-time
✅ **Command Interface**: Send commands (tasks, status, help) to the running simulator
✅ **Configuration Panel**: Set graph ID and number of robots
✅ **Dark Mode Support**: Integrated with existing theme system
✅ **Connection Status**: Visual indicator for WebSocket connection state

## How It Works

1. **Frontend (React + WebSocket)**:
   - User configures simulation parameters
   - Opens WebSocket connection to backend
   - Sends start/stop/command messages
   - Displays real-time output

2. **Backend (Node.js + Express + WebSocket)**:
   - Receives WebSocket messages from frontend
   - Spawns the C++ simulator as a child process
   - Forwards stdin/stdout/stderr between frontend and simulator
   - Handles process lifecycle

3. **C++ Simulator**:
   - Runs in a continuous loop (`while(1)`)
   - Reads commands from stdin
   - Outputs status to stdout
   - Processes tasks dynamically

## Architecture Diagram

```
┌─────────────────┐         WebSocket          ┌──────────────────┐
│                 │◄──────────────────────────►│                  │
│  React Frontend │     (ws://localhost:3001)  │  Express Backend │
│                 │                             │   (Node.js)      │
└─────────────────┘                             └────────┬─────────┘
                                                         │
                                                  spawn  │  stdin/stdout
                                                         │
                                                ┌────────▼─────────┐
                                                │                  │
                                                │  C++ Real-Time   │
                                                │    Simulator     │
                                                │   (while loop)   │
                                                └──────────────────┘
```

## Testing

1. Start both backend and frontend
2. Navigate to http://localhost:5173/realtime
3. Configure: Graph 1, 4 robots
4. Click "Start Simulator"
5. Wait for initialization output
6. Send command: `task 1 5 10`
7. Observe the simulator processing the task
8. Send command: `status`
9. Check robot states
10. Click "Stop Simulator"

## Troubleshooting

**WebSocket won't connect:**
- Make sure backend is running on port 3001
- Check browser console for errors
- Verify no other service is using port 3001

**Simulator won't start:**
- Build the simulator first with `make`
- Check that `optimality/real_time_simulator/build/simulator` exists
- Verify WSL is installed and configured

**No output in console:**
- Wait a few seconds for initialization
- Try sending a `help` command
- Check backend terminal for errors

## File Changes Made

### New Files
- `apps/Simulator/src/pages/RealTimeSimulator.tsx` - Main frontend component
- `apps/Simulator/REALTIME_SIMULATOR.md` - Detailed documentation
- `REALTIME_SETUP.md` - This setup guide

### Modified Files
- `api/server.js` - Added WebSocket support and simulator spawning
- `api/package.json` - Added `ws` dependency
- `apps/Simulator/src/main.tsx` - Added route for /realtime
- `apps/Simulator/src/components/Nav.tsx` - Added "Real-Time" navigation link

## Next Steps

- Test with different graph IDs and robot counts
- Try adding multiple tasks dynamically
- Monitor robot behavior in real-time
- Extend with additional commands if needed
