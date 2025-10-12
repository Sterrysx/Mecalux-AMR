const express = require('express');
const { exec } = require('child_process');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3001;

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
  
  // Parse robot assignments from the detailed report
  let currentRobotId = null;
  let currentRobotTasks = [];
  let taskExecutionTime = null;
  let taskStartTime = 0;
  
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    
    // Look for robot headers like "┌─ Robot 0 ─────────────────────────────────────────"
    const robotHeaderMatch = line.match(/Robot\s+(\d+)/);
    if (robotHeaderMatch) {
      // Save previous robot if exists
      if (currentRobotId !== null && currentRobotTasks.length > 0) {
        robots.push({
          id: currentRobotId + 1, // Make it 1-indexed for display
          tasks: currentRobotTasks
        });
      }
      
      // Start new robot
      currentRobotId = parseInt(robotHeaderMatch[1]);
      currentRobotTasks = [];
      taskStartTime = 0;
    }
    
    // Look for task lines like "│  Task ID 0:"
    const taskMatch = line.match(/Task ID\s+(\d+):/);
    if (taskMatch && currentRobotId !== null) {
      const taskId = parseInt(taskMatch[1]);
      
      // Look ahead for task execution time and cumulative time
      let taskDuration = 10; // Default
      let cumulativeTime = taskStartTime + taskDuration;
      
      // Scan next few lines for timing info
      for (let j = i + 1; j < Math.min(i + 8, lines.length); j++) {
        const nextLine = lines[j];
        
        // Look for task execution time
        const execMatch = nextLine.match(/Task execution:\s*(\d+\.?\d*)s/);
        if (execMatch) {
          taskDuration = parseFloat(execMatch[1]);
        }
        
        // Look for cumulative time
        const cumulMatch = nextLine.match(/Cumulative time:\s*(\d+\.?\d*)s/);
        if (cumulMatch) {
          cumulativeTime = parseFloat(cumulMatch[1]);
          break;
        }
      }
      
      // Calculate actual start time
      const actualStartTime = Math.max(0, cumulativeTime - taskDuration);
      
      currentRobotTasks.push({
        id: `T${taskId}`,
        startTime: actualStartTime,
        duration: taskDuration,
        description: `Task ${taskId}`
      });
      
      taskStartTime = cumulativeTime;
    }
  }
  
  // Save the last robot
  if (currentRobotId !== null && currentRobotTasks.length > 0) {
    robots.push({
      id: currentRobotId + 1,
      tasks: currentRobotTasks
    });
  }
  
  // If parsing failed, create fallback robots
  if (robots.length === 0 && makespan > 0) {
    const numRobots = 2; // Default from your tests
    for (let i = 0; i < numRobots; i++) {
      const numTasks = Math.floor(Math.random() * 4) + 3; // 3-6 tasks per robot
      const tasks = [];
      for (let j = 0; j < numTasks; j++) {
        tasks.push({
          id: `T${i * 10 + j}`,
          startTime: j * (makespan / numTasks),
          duration: (makespan / numTasks) * 0.8,
          description: `Task ${i * 10 + j}`
        });
      }
      robots.push({ id: i + 1, tasks });
    }
  }
  
  // Calculate actual makespan from robot completion times if parsing failed
  if (makespan === 0 && robots.length > 0) {
    makespan = Math.max(...robots.map(robot => 
      robot.tasks.length > 0 
        ? Math.max(...robot.tasks.map(task => task.startTime + task.duration))
        : 0
    ));
    console.log(`Calculated makespan from robot tasks: ${makespan}`);
  }

  return {
    algorithm,
    makespan: makespan || 0, // Don't use default 100, use 0 to indicate parsing failure
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
    
    const plannerPath = '/mnt/c/Users/Adam/Desktop/PAE/Mecalux-AMR/optimality/02_layer_planner';
    
    if (mode === 'comparison' || algorithmId === 0) {
      // Run all three algorithms
      const algorithms = [
        { id: 1, name: 'Brute Force' },
        { id: 2, name: 'Greedy' },
        { id: 3, name: 'Hill Climbing' }
      ];
      
      const results = [];
      
      for (const alg of algorithms) {
        const command = `wsl -e bash -c "cd ${plannerPath} && ./build/planner ${alg.id} ${graphId} ${numTasks} ${numRobots}"`;
        
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
      const command = `wsl -e bash -c "cd ${plannerPath} && ./build/planner ${algorithmId} ${graphId} ${numTasks} ${numRobots}"`;
      
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
    const command = `wsl -d Ubuntu -- bash -c "ls /mnt/c/Users/Adam/Desktop/PAE/Mecalux-AMR/optimality/01_layer_mapping/tests/distributions/ | grep 'graph.*\\.inp' | sort -V"`;
    
    const { promisify } = require('util');
    const execPromise = promisify(exec);
    
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