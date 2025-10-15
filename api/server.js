const express = require('express');
const { exec } = require('child_process');
const cors = require('cors');
const path = require('path');
const os = require('os');

const app = express();
const PORT = 3001;

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
    currentRobotTasks.push(currentTaskData);
  }
  if (currentRobotId !== null) {
    robots.push({
      id: currentRobotId + 1,
      tasks: currentRobotTasks,
      initialBattery: currentRobotInitialBattery,
      finalBattery: currentRobotFinalBattery,
      completionTime: currentRobotCompletionTime
    });
  }
  
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
          exec(command, { timeout: 30000 }, (error, stdout, stderr) => {
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
      
    } else {
      // Run single algorithm
      const command = buildPlannerCommand(algorithmId, graphId, numTasks, numRobots);
      console.log(`Running command: ${command}`);
      
      exec(command, { timeout: 30000 }, (error, stdout, stderr) => {
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

app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Planner API server running on http://localhost:${PORT}`);
  console.log(`🔗 Use this server with your React frontend`);
  console.log(`📡 Server listening on all interfaces: 0.0.0.0:${PORT}`);
});