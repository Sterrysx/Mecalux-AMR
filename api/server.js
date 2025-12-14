const express = require('express');
const { exec, spawn } = require('child_process');
const cors = require('cors');
const path = require('path');
const os = require('os');
const http = require('http');
const WebSocket = require('ws');
const fs = require('fs').promises;

const app = express();
const PORT = 3001;
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Output directory for fleet data
const OUTPUT_DIR = path.resolve(__dirname, './output');
const ORCA_DIR = path.resolve(__dirname, './orca');

// Function to find and read the latest orca_tick_*.json file
async function getLatestOrcaTick() {
  try {
    // Read all files in the orca directory
    const files = await fs.readdir(ORCA_DIR);
    
    // Filter and extract tick numbers
    const tickFiles = files
      .filter(file => file.startsWith('orca_tick_') && file.endsWith('.json'))
      .map(file => {
        const match = file.match(/orca_tick_(\d+)\.json/);
        return match ? { filename: file, tick: parseInt(match[1], 10) } : null;
      })
      .filter(item => item !== null);
    
    // If no tick files found, return null
    if (tickFiles.length === 0) {
      return null;
    }
    
    // Sort by tick number descending and get the highest
    tickFiles.sort((a, b) => b.tick - a.tick);
    const latestFile = tickFiles[0];
    
    // Read and parse the latest file
    const filePath = path.join(ORCA_DIR, latestFile.filename);
    const data = await fs.readFile(filePath, 'utf8');
    return JSON.parse(data);
  } catch (error) {
    console.error('Error reading latest orca tick:', error);
    return null;
  }
}

// OS Detection and Path Configuration
const isWindows = os.platform() === 'win32';
const PLANNER_DIR = path.resolve(__dirname, '../optimality/02_layer_planner');
const GRAPH_DIR = path.resolve(__dirname, '../optimality/01_layer_mapping/tests/distributions');

// Helper function to build planner command based on OS
function buildPlannerCommand(algorithmId, graphId, numTasks, numRobots) {
  const args = `${algorithmId} ${graphId} ${numTasks} ${numRobots}`;
  
  if (isWindows) {
    // On Windows, use WSL with proper path conversion
    const wslPlannerPath = PLANNER_DIR.replace(/\\/g, '/').replace(/^([A-Z]):/, (match, drive) => `/mnt/${drive.toLowerCase()}`);
    return `wsl -e bash -c "cd ${wslPlannerPath} && ./build/planner ${args}"`;
  } else {
    // On Linux/macOS, run directly
    return `bash -c "cd ${PLANNER_DIR} && ./build/planner ${args}"`;
  }
}

// Middleware
app.use(cors());
app.use(express.json());

// Parse planner output to extract results
function parseOutput(output, algorithm) {
  const lines = output.split('\n').filter(line => line.trim());
  
  let makespan = 0;
  let computationTime = 0;
  let robots = [];
  
  // Extract makespan and computation time from output
  for (const line of lines) {
    // Look for "Makespan (max robot completion time): 39.40 seconds."
    if (line.includes('Makespan') && (line.includes('seconds') || line.includes('completion time'))) {
      const match = line.match(/(\d+\.?\d*)\s*seconds/i);
      if (match) {
        makespan = parseFloat(match[1]);
        console.log(`Found makespan: ${makespan} from line: "${line}"`);
      }
    }
    
    // Look for "Algorithm computation time: 0.2169 ms"
    if (line.includes('computation time') && line.includes('ms')) {
      const match = line.match(/(\d+\.?\d*)\s*ms/i);
      if (match) {
        computationTime = parseFloat(match[1]);
        console.log(`Found computation time: ${computationTime} from line: "${line}"`);
      }
    }
  }
  
  // Parse robot assignments from the detailed report with ALL information
  let currentRobotId = null;
  let currentRobotTasks = [];
  let currentRobotInitialBattery = 100;
  let currentRobotFinalBattery = 100;
  let currentRobotCompletionTime = 0;
  let currentTaskData = null;
  let currentChargingData = null;
  let chargingDecisionData = null;
  
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    
    // Look for robot headers like "┌─ Robot 0 ─────────────────────────────────────────"
    const robotHeaderMatch = line.match(/Robot\s+(\d+)/);
    if (robotHeaderMatch && line.includes('─')) {
      // Save previous robot if exists
      if (currentRobotId !== null) {
        // Save any pending task before saving the robot
        if (currentTaskData) {
          currentRobotTasks.push(currentTaskData);
          currentTaskData = null;
        }
        
        robots.push({
          id: currentRobotId + 1,
          tasks: currentRobotTasks,
          initialBattery: currentRobotInitialBattery,
          finalBattery: currentRobotFinalBattery,
          completionTime: currentRobotCompletionTime
        });
      }
      
      // Start new robot
      currentRobotId = parseInt(robotHeaderMatch[1]);
      currentRobotTasks = [];
      currentRobotInitialBattery = 100;
      currentRobotFinalBattery = 100;
      currentRobotCompletionTime = 0;
      currentTaskData = null;
      currentChargingData = null;
      chargingDecisionData = null;
    }
    
    // Parse initial battery
    const initialBatteryMatch = line.match(/Initial battery:\s*(\d+\.?\d*)%/);
    if (initialBatteryMatch) {
      currentRobotInitialBattery = parseFloat(initialBatteryMatch[1]);
    }
    
    // Parse final battery
    const finalBatteryMatch = line.match(/Final battery level:\s*(\d+\.?\d*)%/);
    if (finalBatteryMatch) {
      currentRobotFinalBattery = parseFloat(finalBatteryMatch[1]);
    }
    
    // Parse total completion time
    const completionMatch = line.match(/Total completion time:\s*(\d+\.?\d*)s/);
    if (completionMatch) {
      currentRobotCompletionTime = parseFloat(completionMatch[1]);
    }
    
    // Look for task lines like "│  Task ID 0:"
    const taskMatch = line.match(/Task ID\s+(\d+):/);
    if (taskMatch && currentRobotId !== null) {
      // If we have a previous task, save it
      if (currentTaskData) {
        currentRobotTasks.push(currentTaskData);
      }
      
      // Start new task
      currentTaskData = {
        id: `T${taskMatch[1]}`,
        taskId: parseInt(taskMatch[1]),
        fromNode: null,
        fromCoords: null,
        toNode: null,
        toCoords: null,
        travelTime: 0,
        executionTime: 0,
        cumulativeTime: 0,
        batteryLevel: 100,
        startTime: 0,
        duration: 0
      };
    }
    
    // Parse From node
    const fromMatch = line.match(/From:\s*Node\s+(\d+)\s*\((\d+\.?\d*),\s*(\d+\.?\d*)\)/);
    if (fromMatch && currentTaskData) {
      currentTaskData.fromNode = parseInt(fromMatch[1]);
      currentTaskData.fromCoords = { x: parseFloat(fromMatch[2]), y: parseFloat(fromMatch[3]) };
    }
    
    // Parse To node
    const toMatch = line.match(/To:\s*Node\s+(\d+)\s*\((\d+\.?\d*),\s*(\d+\.?\d*)\)/);
    if (toMatch && currentTaskData) {
      currentTaskData.toNode = parseInt(toMatch[1]);
      currentTaskData.toCoords = { x: parseFloat(toMatch[2]), y: parseFloat(toMatch[3]) };
    }
    
    // Parse travel time
    const travelMatch = line.match(/Travel to origin:\s*(\d+\.?\d*)s/);
    if (travelMatch && currentTaskData) {
      currentTaskData.travelTime = parseFloat(travelMatch[1]);
    }
    
    // Parse execution time
    const execMatch = line.match(/Task execution:\s*(\d+\.?\d*)s/);
    if (execMatch && currentTaskData) {
      currentTaskData.executionTime = parseFloat(execMatch[1]);
      currentTaskData.duration = currentTaskData.executionTime;
    }
    
    // Parse cumulative time
    const cumulMatch = line.match(/Cumulative time:\s*(\d+\.?\d*)s/);
    if (cumulMatch && currentTaskData) {
      currentTaskData.cumulativeTime = parseFloat(cumulMatch[1]);
      currentTaskData.startTime = Math.max(0, currentTaskData.cumulativeTime - currentTaskData.executionTime);
    }
    
    // Parse battery level
    const batteryMatch = line.match(/Battery level:\s*(\d+\.?\d*)%/);
    if (batteryMatch && currentTaskData) {
      currentTaskData.batteryLevel = parseFloat(batteryMatch[1]);
    }
    
    // Parse CHARGING DECISION (Preventive)
    if (line.includes('CHARGING DECISION') && line.includes('Preventive')) {
      // Save current task if exists
      if (currentTaskData && currentRobotId !== null) {
        currentRobotTasks.push(currentTaskData);
        currentTaskData = null;
      }
      
      // Initialize charging decision data
      chargingDecisionData = {
        currentBattery: 0,
        nextTaskId: 0,
        nextTaskConsumption: 0,
        batteryAfterTask: 0,
        threshold: 20.0
      };
    }
    
    // Parse charging decision details
    if (chargingDecisionData) {
      const currentBatteryMatch = line.match(/Current battery:\s*(\d+\.?\d*)%/);
      if (currentBatteryMatch) {
        chargingDecisionData.currentBattery = parseFloat(currentBatteryMatch[1]);
      }
      
      const nextTaskMatch = line.match(/Next task \(ID\s+(\d+)\)\s+would consume:\s*(\d+\.?\d*)%/);
      if (nextTaskMatch) {
        chargingDecisionData.nextTaskId = parseInt(nextTaskMatch[1]);
        chargingDecisionData.nextTaskConsumption = parseFloat(nextTaskMatch[2]);
      }
      
      const batteryAfterMatch = line.match(/Battery after task would be:\s*(\d+\.?\d*)%/);
      if (batteryAfterMatch) {
        chargingDecisionData.batteryAfterTask = parseFloat(batteryAfterMatch[1]);
      }
      
      const thresholdMatch = line.match(/BELOW\s+(\d+\.?\d*)%\s+threshold/);
      if (thresholdMatch) {
        chargingDecisionData.threshold = parseFloat(thresholdMatch[1]);
      }
    }
    
    // Parse CHARGING EVENT
    if (line.includes('CHARGING EVENT')) {
      // Initialize charging event
      currentChargingData = {
        type: 'charging',
        batteryBefore: 0,
        batteryAfter: 100,
        travelTime: 0,
        chargingTime: 0,
        startTime: 0,
        duration: 0,
        decision: chargingDecisionData
      };
      chargingDecisionData = null; // Reset decision data
    }
    
    // Parse charging event details
    if (currentChargingData) {
      const travelToChargerMatch = line.match(/Travel to charging station.*:\s*(\d+\.?\d*)s/);
      if (travelToChargerMatch) {
        currentChargingData.travelTime = parseFloat(travelToChargerMatch[1]);
      }
      
      const batteryOnArrivalMatch = line.match(/Battery on arrival:\s*(\d+\.?\d*)%/);
      if (batteryOnArrivalMatch) {
        currentChargingData.batteryBefore = parseFloat(batteryOnArrivalMatch[1]);
      }
      
      const chargingTimeMatch = line.match(/Charging time:\s*(\d+\.?\d*)s/);
      if (chargingTimeMatch) {
        currentChargingData.chargingTime = parseFloat(chargingTimeMatch[1]);
        currentChargingData.duration = currentChargingData.travelTime + currentChargingData.chargingTime;
      }
      
      const batteryAfterChargingMatch = line.match(/Battery after charging:\s*(\d+\.?\d*)%/);
      if (batteryAfterChargingMatch) {
        currentChargingData.batteryAfter = parseFloat(batteryAfterChargingMatch[1]);
        
        // Calculate start time based on last task or event
        if (currentRobotTasks.length > 0) {
          const lastItem = currentRobotTasks[currentRobotTasks.length - 1];
          currentChargingData.startTime = lastItem.type === 'charging' 
            ? lastItem.startTime + lastItem.duration 
            : lastItem.cumulativeTime;
        }
        
        // Save charging event and reset
        currentRobotTasks.push(currentChargingData);
        currentChargingData = null;
      }
    }
  }
  
  // Save the last task and robot
  if (currentTaskData && currentRobotId !== null) {
    console.log(`Saving final task for robot ${currentRobotId}: Task ${currentTaskData.taskId}`);
    currentRobotTasks.push(currentTaskData);
  }
  if (currentRobotId !== null) {
    console.log(`Saving final robot ${currentRobotId} with ${currentRobotTasks.length} tasks`);
    robots.push({
      id: currentRobotId + 1,
      tasks: currentRobotTasks,
      initialBattery: currentRobotInitialBattery,
      finalBattery: currentRobotFinalBattery,
      completionTime: currentRobotCompletionTime
    });
  }
  
  console.log(`Total robots parsed: ${robots.length}`);
  console.log(`Total tasks across all robots: ${robots.reduce((sum, r) => sum + r.tasks.length, 0)}`);
  robots.forEach((robot, idx) => {
    console.log(`  Robot ${robot.id}: ${robot.tasks.length} tasks - ${robot.tasks.map(t => t.type === 'charging' ? 'CHARGE' : `T${t.taskId}`).join(', ')}`);
  });
  
  // Calculate actual makespan from robot completion times if parsing failed
  if (makespan === 0 && robots.length > 0) {
    makespan = Math.max(...robots.map(robot => robot.completionTime || 0));
    console.log(`Calculated makespan from robot tasks: ${makespan}`);
  }

  return {
    algorithm,
    makespan: makespan || 0,
    computationTime: computationTime || 0,
    robots,
    rawOutput: output
  };
}

// Main planner endpoint
app.post('/api/planner', async (req, res) => {
  try {
    const { algorithmId, graphId, numTasks, numRobots, mode } = req.body;
    
    console.log('Received request:', { algorithmId, graphId, numTasks, numRobots, mode });
    
    if (mode === 'comparison' || algorithmId === 0) {
      // Run all three algorithms
      const algorithms = [
        { id: 1, name: 'Brute Force' },
        { id: 2, name: 'Greedy' },
        { id: 3, name: 'Hill Climbing' }
      ];
      
      const results = [];
      
      for (const alg of algorithms) {
        const command = buildPlannerCommand(alg.id, graphId, numTasks, numRobots);
        console.log(`Running command: ${command}`);
        
        await new Promise((resolve, reject) => {
          exec(command, { timeout: 300000 }, (error, stdout, stderr) => {
            if (error) {
              console.error(`Error running ${alg.name}:`, error);
              // Use fallback result on error
              results.push(parseOutput('', alg.name));
            } else {
              console.log(`${alg.name} output:`, stdout);
              results.push(parseOutput(stdout, alg.name));
            }
            resolve();
          });
        });
      }
      
      // Calculate improvements relative to first algorithm
      const baseline = results[0];
      const comparison = results.map((result, idx) => ({
        algorithm: result.algorithm,
        makespan: result.makespan,
        computationTime: result.computationTime,
        improvement: idx === 0 ? null : ((baseline.makespan - result.makespan) / baseline.makespan * 100)
      }));
      
      // Find best result (lowest makespan)
      const bestResult = results.reduce((best, current) => 
        current.makespan < best.makespan ? current : best
      );
      
      res.json({
        ...bestResult,
        comparison
      });
      
    } else if (algorithmId === -1) {
      // Run heuristics only (Greedy and Hill Climbing)
      const algorithms = [
        { id: 2, name: 'Greedy' },
        { id: 3, name: 'Hill Climbing' }
      ];
      
      const results = [];
      
      for (const alg of algorithms) {
        const command = buildPlannerCommand(alg.id, graphId, numTasks, numRobots);
        console.log(`Running command: ${command}`);
        
        await new Promise((resolve, reject) => {
          exec(command, { timeout: 300000 }, (error, stdout, stderr) => {
            if (error) {
              console.error(`Error running ${alg.name}:`, error);
              results.push(parseOutput('', alg.name));
            } else {
              console.log(`${alg.name} output:`, stdout);
              results.push(parseOutput(stdout, alg.name));
            }
            resolve();
          });
        });
      }

      // Calculate improvements relative to Greedy
      const baseline = results[0];
      const comparison = results.map((result, idx) => ({
        algorithm: result.algorithm,
        makespan: result.makespan,
        computationTime: result.computationTime,
        improvement: idx === 0 ? null : ((baseline.makespan - result.makespan) / baseline.makespan * 100)
      }));

      // Find best result (lowest makespan)
      const bestResult = results.reduce((best, current) =>
        current.makespan < best.makespan ? current : best
      );

      res.json({
        ...bestResult,
        comparison
      });
      
    } else {
      // Run single algorithm
      const command = buildPlannerCommand(algorithmId, graphId, numTasks, numRobots);
      console.log(`Running command: ${command}`);
      
      exec(command, { timeout: 300000 }, (error, stdout, stderr) => {
        if (error) {
          console.error('Error running planner:', error);
          return res.status(500).json({ 
            error: 'Failed to run planner', 
            details: error.message 
          });
        }
        
        console.log('Planner output:', stdout);
        if (stderr) console.log('Planner stderr:', stderr);
        
        const algorithmNames = {
          '1': 'Brute Force',
          '2': 'Greedy', 
          '3': 'Hill Climbing'
        };
        
        try {
          const result = parseOutput(stdout, algorithmNames[algorithmId] || 'Unknown');
          res.json(result);
        } catch (parseError) {
          console.error('Error parsing output:', parseError);
          res.status(500).json({ 
            error: 'Failed to parse planner output', 
            details: parseError.message,
            rawOutput: stdout
          });
        }
      });
    }
    
  } catch (error) {
    console.error('Server error:', error);
    res.status(500).json({ 
      error: 'Internal server error', 
      details: error.message 
    });
  }
});

// Get available graphs endpoint
app.get('/api/graphs', async (req, res) => {
  try {
    const { promisify } = require('util');
    const execPromise = promisify(exec);
    const fs = require('fs').promises;
    
    // Try to read graphs directly from filesystem first (works on Linux/macOS/Windows)
    try {
      const files = await fs.readdir(GRAPH_DIR);
      const graphs = files
        .filter(file => file.match(/^graph\d+\.inp$/))
        .map(file => {
          const match = file.match(/graph(\d+)\.inp/);
          return match ? parseInt(match[1]) : null;
        })
        .filter(id => id !== null)
        .sort((a, b) => a - b);
      
      if (graphs.length > 0) {
        return res.json({ graphs });
      }
    } catch (fsError) {
      console.log('Direct filesystem read failed, trying command-based approach:', fsError.message);
    }
    
    // Fallback: use command-based approach for WSL
    let command;
    if (isWindows) {
      const wslGraphPath = GRAPH_DIR.replace(/\\/g, '/').replace(/^([A-Z]):/, (match, drive) => `/mnt/${drive.toLowerCase()}`);
      command = `wsl -e bash -c "ls ${wslGraphPath} | grep 'graph.*\\.inp' | sort -V"`;
    } else {
      command = `bash -c "ls ${GRAPH_DIR} | grep 'graph.*\\.inp' | sort -V"`;
    }
    
    const { stdout } = await execPromise(command);
    const graphs = stdout.trim().split('\n')
      .filter(file => file.match(/^graph\d+\.inp$/))
      .map(file => {
        const match = file.match(/graph(\d+)\.inp/);
        return match ? parseInt(match[1]) : null;
      })
      .filter(id => id !== null)
      .sort((a, b) => a - b);
    
    res.json({ graphs });
  } catch (error) {
    console.error('Error getting available graphs:', error);
    // Fallback to default graphs 1-10
    res.json({ graphs: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10] });
  }
});

// Health check endpoint
app.get('/api/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// WebSocket handler for real-time simulator
let simulatorProcess = null;
let currentRobotData = { robots: [], lastUpdate: Date.now() };
let connectedClients = new Set();

// Broadcast to all connected WebSocket clients
function broadcast(message) {
  const messageStr = JSON.stringify(message);
  connectedClients.forEach(client => {
    if (client.readyState === 1) { // WebSocket.OPEN
      client.send(messageStr);
    }
  });
}

// Parse robot state from backend output
function parseRobotState(output) {
  const lines = output.split('\n');
  const robots = [];
  
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    // Look for robot state lines like: "0  | BUSY   | 9206    | (1467, 207) | COLLISION_WAIT | 83 |"
    const match = line.match(/^(\d+)\s*\|\s*(\w+)\s*\|\s*(\d+)\s*\|\s*\((\d+),\s*(\d+)\)\s*\|\s*(\w+)\s*\|\s*(\d+)\s*\|/);
    if (match) {
      const [_, id, status, node, x, y, l3state, itinerary] = match;
      robots.push({
        id: parseInt(id),
        x: parseInt(x),
        y: parseInt(y),
        vx: 0, // velocity not in status output
        vy: 0,
        state: l3state,
        goal: `Node ${node}`,
        itinerary: parseInt(itinerary),
        batteryLevel: 100 // battery not in status output, default to full
      });
    }
  }
  
  if (robots.length > 0) {
    currentRobotData = { robots, lastUpdate: Date.now() };
  }
}

wss.on('connection', (ws) => {
  console.log('WebSocket client connected');
  connectedClients.add(ws);
  
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      
      if (data.type === 'start_simulator') {
        // Prevent starting multiple instances
        if (simulatorProcess) {
          console.log('Simulator already running, ignoring start request');
          return;
        }
        
        // Start the fleet manager
        const { graphId, numRobots } = data;
        console.log(`Starting Fleet Manager: ${numRobots} robots`);
        
        const SIMULATOR_DIR = path.resolve(__dirname, '../backend');
        let command;
        
        if (isWindows) {
          const wslSimulatorPath = SIMULATOR_DIR.replace(/\\/g, '/').replace(/^([A-Z]):/, (match, drive) => `/mnt/${drive.toLowerCase()}`);
          command = `wsl`;
          // Run fleet_manager with --cli flag for interactive mode
          const args = ['-e', 'bash', '-c', `cd ${wslSimulatorPath} && ./build/fleet_manager --cli --robots ${numRobots}`];
          simulatorProcess = spawn(command, args);
        } else {
          // Run fleet_manager with --cli flag for interactive mode
          simulatorProcess = spawn('./build/fleet_manager', ['--cli', '--robots', numRobots.toString()], {
            cwd: SIMULATOR_DIR
          });
        }
        
        // Send stdout to ALL clients
        simulatorProcess.stdout.on('data', (data) => {
          const output = data.toString();
          console.log('Simulator output:', output);
          console.log(`Broadcasting to ${connectedClients.size} clients`);
          
          // Parse robot state from output
          parseRobotState(output);
          
          broadcast({
            type: 'simulator_output',
            data: output
          });
        });
        
        // Send stderr to ALL clients
        simulatorProcess.stderr.on('data', (data) => {
          const error = data.toString();
          console.error('Simulator error:', error);
          broadcast({
            type: 'simulator_error',
            data: error
          });
        });
        
        // Handle process exit
        simulatorProcess.on('close', (code) => {
          console.log(`Simulator process exited with code ${code}`);
          ws.send(JSON.stringify({
            type: 'simulator_closed',
            code
          }));
          simulatorProcess = null;
        });
        
        ws.send(JSON.stringify({
          type: 'simulator_started',
          message: 'Real-time simulator started successfully'
        }));
      } else if (data.type === 'send_command') {
        // Send command to the running simulator
        if (simulatorProcess && simulatorProcess.stdin) {
          console.log('Sending command to simulator:', data.command);
          simulatorProcess.stdin.write(data.command + '\n');
        } else {
          ws.send(JSON.stringify({
            type: 'error',
            message: 'Simulator is not running'
          }));
        }
      } else if (data.type === 'stop_simulator') {
        // Stop the simulator
        if (simulatorProcess) {
          console.log('Stopping simulator');
          simulatorProcess.kill();
          simulatorProcess = null;
          ws.send(JSON.stringify({
            type: 'simulator_stopped',
            message: 'Simulator stopped'
          }));
        }
      }
    } catch (error) {
      console.error('WebSocket message error:', error);
      ws.send(JSON.stringify({
        type: 'error',
        message: error.message
      }));
    }
  });
  
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    connectedClients.delete(ws);
    
    // Only clean up simulator if NO clients are left
    if (connectedClients.size === 0 && simulatorProcess) {
      console.log('No clients left - cleaning up simulator process');
      simulatorProcess.kill();
      simulatorProcess = null;
    }
  });
});

// ==================== FLEET MANAGEMENT API ====================

// Get current robot data from latest orca tick
app.get('/api/fleet/robots', async (req, res) => {
  try {
    const latestData = await getLatestOrcaTick();
    
    if (!latestData) {
      // Fallback: return waiting state if no tick files found
      return res.json({
        robots: [],
        timestamp: Date.now(),
        status: 'waiting',
        message: 'Waiting for simulation to start'
      });
    }
    
    // Transform orca format to expected format
    const robotsData = {
      robots: latestData.robots.map(robot => ({
        id: robot.id,
        x: robot.x,
        y: robot.y,
        vx: robot.vx,
        vy: robot.vy,
        state: robot.status || robot.driverState,
        goal: robot.targetNodeId,
        itinerary: robot.remainingWaypoints ? [robot.remainingWaypoints] : [],
        batteryLevel: Math.round(robot.battery * 100)
      })),
      timestamp: new Date(latestData.timestamp).getTime(),
      tick: latestData.tick
    };
    
    res.json(robotsData);
  } catch (error) {
    console.error('Error serving robot data:', error);
    res.status(500).json({ error: 'Failed to fetch robot data' });
  }
});

// Get latest orca tick (raw format)
app.get('/api/orca/latest', async (req, res) => {
  try {
    const latestData = await getLatestOrcaTick();
    
    if (!latestData) {
      return res.status(404).json({
        error: 'No simulation data available',
        message: 'Waiting for ORCA simulation to generate tick files'
      });
    }
    
    res.json(latestData);
  } catch (error) {
    console.error('Error serving latest orca tick:', error);
    res.status(500).json({ error: 'Failed to fetch latest tick' });
  }
});

// Serve static JSON files from output directory
app.use('/api/output', express.static(OUTPUT_DIR));

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: Date.now() });
});

// Get POIs configuration
app.get('/api/pois.json', async (req, res) => {
  try {
    const poiConfigPath = path.resolve(__dirname, '../backend/layer1/assets/poi_config.json');
    const data = await fs.readFile(poiConfigPath, 'utf8');
    const config = JSON.parse(data);
    
    // Transform to frontend format
    const pois = config.pois.map(poi => ({
      id: poi.id,
      type: poi.type,
      x: poi.x,
      y: poi.y,
      nodeId: poi.nodeId || 0
    }));
    
    res.json(pois);
  } catch (error) {
    console.error('Error loading POIs:', error);
    res.status(500).json({ error: 'Failed to load POIs' });
  }
});

// Inject new tasks
app.post('/api/fleet/inject-tasks', async (req, res) => {
  try {
    const { tasks } = req.body;
    
    if (!Array.isArray(tasks)) {
      return res.status(400).json({ error: 'Tasks must be an array' });
    }
    
    // Determine scenario based on task count
    const scenario = tasks.length <= 5 ? 'B' : 'C';
    
    console.log(`[Fleet API] Injecting ${tasks.length} tasks (Scenario ${scenario})`);
    
    // TODO: Send to backend C++ fleet manager via IPC or file
    // For now, return success with scenario info
    
    res.json({
      success: true,
      tasksInjected: tasks.length,
      scenario: scenario,
      message: scenario === 'B' 
        ? 'Cheap Insertion (≤5 tasks)' 
        : 'Background Re-plan (>5 tasks with starter tasks)'
    });
  } catch (error) {
    console.error('Error injecting tasks:', error);
    res.status(500).json({ error: 'Failed to inject tasks' });
  }
});

// Send robot to charging station
app.post('/api/fleet/robot/:robotId/charge', async (req, res) => {
  try {
    const { robotId } = req.params;
    const { chargingStationId } = req.body;
    
    console.log(`[Fleet API] Sending robot ${robotId} to charging station ${chargingStationId}`);
    
    // TODO: Send command to backend
    
    res.json({ 
      success: true, 
      robotId: parseInt(robotId), 
      chargingStationId 
    });
  } catch (error) {
    console.error('Error sending robot to charge:', error);
    res.status(500).json({ error: 'Failed to send robot to charge' });
  }
});

// Get system statistics
app.get('/api/fleet/stats', async (req, res) => {
  try {
    // TODO: Read actual stats from backend
    // For now, return mock data
    
    res.json({
      currentScenario: 'B',
      vrpSolverActive: false,
      lastVRPSolveTime: 124,
      avgPathQueryTime: 48,
      physicsLoopHz: 20,
      throughput: 23
    });
  } catch (error) {
    console.error('Error fetching stats:', error);
    res.status(500).json({ error: 'Failed to fetch stats' });
  }
});

// Initialize output directory with mock data
async function initializeMockData() {
  try {
    await fs.mkdir(OUTPUT_DIR, { recursive: true });
    
    // Create mock robots.json
    const robotsData = {
      robots: [
        { id: 0, x: 1467, y: 207, vx: 0, vy: 0, state: 'IDLE', goal: null, itinerary: [], batteryLevel: 95 },
        { id: 1, x: 1467, y: 257, vx: 0, vy: 0, state: 'IDLE', goal: null, itinerary: [], batteryLevel: 88 },
        { id: 2, x: 1467, y: 307, vx: 0, vy: 0, state: 'IDLE', goal: null, itinerary: [], batteryLevel: 92 },
        { id: 3, x: 1467, y: 357, vx: 0, vy: 0, state: 'IDLE', goal: null, itinerary: [], batteryLevel: 76 }
      ],
      timestamp: Date.now()
    };
    
    // Create mock tasks.json
    const tasksData = {
      tasks: [],
      timestamp: Date.now()
    };
    
    // Create mock map.json
    const mapData = {
      obstacles: [],
      timestamp: Date.now()
    };
    
    await fs.writeFile(path.join(OUTPUT_DIR, 'robots.json'), JSON.stringify(robotsData, null, 2));
    await fs.writeFile(path.join(OUTPUT_DIR, 'tasks.json'), JSON.stringify(tasksData, null, 2));
    await fs.writeFile(path.join(OUTPUT_DIR, 'map.json'), JSON.stringify(mapData, null, 2));
    
    console.log('✅ Initialized mock fleet data in', OUTPUT_DIR);
  } catch (error) {
    console.error('Error initializing mock data:', error);
  }
}

// Initialize on startup
initializeMockData();

server.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Planner API server running on http://localhost:${PORT}`);
  console.log(`🔗 Use this server with your React frontend`);
  console.log(`📡 Server listening on all interfaces: 0.0.0.0:${PORT}`);
  console.log(`📊 Fleet API endpoints:`);
  console.log(`   GET  /health - Health check`);
  console.log(`   GET  /api/fleet/robots - Latest robot data (from orca ticks)`);
  console.log(`   GET  /api/orca/latest - Latest raw orca tick data`);
  console.log(`   GET  /api/output/robots.json - Robot positions (legacy)`);
  console.log(`   GET  /api/output/tasks.json - Task statuses (1 Hz)`);
  console.log(`   GET  /api/output/map.json - Dynamic obstacles (1 Hz)`);
  console.log(`   GET  /api/pois.json - POI configuration`);
  console.log(`   POST /api/fleet/inject-tasks - Inject new tasks`);
  console.log(`   GET  /api/fleet/stats - System statistics`);
});