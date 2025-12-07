# Real-Time Simulator Frontend

This frontend interface allows you to interact with the real-time warehouse robot simulator through a web browser.

## Features

- **WebSocket Communication**: Real-time bidirectional communication with the simulator
- **Interactive Console**: View live output from the simulator
- **Dynamic Task Assignment**: Add tasks to the simulation while it's running
- **Configuration Panel**: Set graph ID and number of robots
- **Command Interface**: Send commands to the simulator (task, help, status, quit)

## Setup

### Prerequisites

1. **Backend API Server** must be running with WebSocket support
2. **Real-Time Simulator** must be built in `optimality/real_time_simulator/build/`

### Installation

```bash
# Install backend dependencies
cd api
npm install

# Install frontend dependencies
cd apps/Simulator
npm install
```

### Running

1. **Start the backend server**:
```bash
cd api
node server.js
```
The server will start on `http://localhost:3001` with WebSocket support on `ws://localhost:3001`

2. **Start the frontend**:
```bash
cd apps/Simulator
npm run dev
```
The frontend will be available at `http://localhost:5173`

3. **Navigate to Real-Time Simulator**:
   - Open your browser to `http://localhost:5173/realtime`
   - Or click on the "Real-Time" link in the navigation bar

## Usage

### Starting the Simulator

1. Select a **Graph ID** (1-10) - This determines the warehouse layout
2. Set the **Number of Robots** (1-10) - How many robots to simulate
3. Click **🚀 Start Simulator**

The simulator will:
- Load the specified graph
- Initialize the robots
- Begin the main simulation loop
- Display real-time output in the console

### Sending Commands

While the simulator is running, you can send commands:

- **Add a task**: `task <id> <origin> <destination>`
  - Example: `task 1 5 15` - Create task ID 1 from node 5 to node 15
  
- **Show help**: `help`
  - Displays available commands
  
- **Check status**: `status`
  - Shows current simulation state and robot information
  
- **Exit**: `quit`
  - Gracefully stops the simulator

### Stopping the Simulator

Click **⏹️ Stop Simulator** to terminate the simulation at any time.

## Architecture

### Frontend Components

**RealTimeSimulator.tsx**:
- Manages WebSocket connection to the backend
- Handles user input and configuration
- Displays real-time output from the simulator
- Sends commands to the running simulator

### Backend WebSocket Handlers

The server (`api/server.js`) provides WebSocket endpoints that:

1. **Start Simulator**: Spawns the real-time simulator process
2. **Send Commands**: Forwards commands to the simulator's stdin
3. **Stream Output**: Sends stdout/stderr back to the frontend
4. **Handle Errors**: Captures and reports errors

### Message Protocol

**Client → Server**:
```json
{
  "type": "start_simulator",
  "graphId": 1,
  "numRobots": 4
}

{
  "type": "send_command",
  "command": "task 1 5 15"
}

{
  "type": "stop_simulator"
}
```

**Server → Client**:
```json
{
  "type": "simulator_started",
  "message": "Real-time simulator started successfully"
}

{
  "type": "simulator_output",
  "data": "Robot 0: Executing task 1..."
}

{
  "type": "simulator_error",
  "data": "Error message"
}

{
  "type": "simulator_closed",
  "code": 0
}
```

## Troubleshooting

### WebSocket Connection Failed
- Ensure the backend server is running on port 3001
- Check that no firewall is blocking WebSocket connections
- Verify the WebSocket URL in `RealTimeSimulator.tsx` matches your server

### Simulator Not Starting
- Build the real-time simulator first:
  ```bash
  cd optimality/real_time_simulator
  make
  ```
- Check that `./build/simulator` exists and is executable
- Verify graph files exist in `optimality/01_layer_mapping/tests/distributions/`

### Commands Not Working
- Ensure the simulator is running (green "Running" status)
- Check the simulator's `USAGE_EXAMPLES.md` for command syntax
- Look at the output console for error messages

## Development

### Adding New Commands

1. Update the simulator's command parser in `SimulationController.cc`
2. Document the new command in the frontend UI
3. No backend changes needed - commands are forwarded transparently

### Customizing the UI

The component uses Tailwind CSS and supports dark mode through the `ThemeContext`.

Edit `src/pages/RealTimeSimulator.tsx` to:
- Add new configuration options
- Customize the output display
- Add visualization features

## See Also

- `optimality/real_time_simulator/README.md` - Real-time simulator documentation
- `optimality/real_time_simulator/USAGE_EXAMPLES.md` - Command examples
- `optimality/real_time_simulator/ARCHITECTURE.md` - Technical architecture
