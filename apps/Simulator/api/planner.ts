// API endpoint to run the Task Planner
// This would be implemented in your backend (Node.js/Express, Python Flask, etc.)
// For demonstration, here's how it could work:

import { exec } from 'child_process';
import { promisify } from 'util';

const execAsync = promisify(exec);

interface PlannerParams {
  algorithmId: number;
  graphId: number;
  numTasks: number;
  numRobots: number;
}

interface PlannerResult {
  algorithm: string;
  makespan: number;
  computationTime: number;
  robots: Array<{
    id: number;
    completionTime: number;
    finalBattery: number;
    tasks: Array<{
      taskId: number;
      fromNode: number;
      toNode: number;
      travelTime: number;
      executionTime: number;
      cumulativeTime: number;
      batteryLevel: number;
    }>;
  }>;
}

export async function runPlanner(params: PlannerParams): Promise<PlannerResult> {
  const { algorithmId, graphId, numTasks, numRobots } = params;
  
  // Construct WSL command
  const command = `wsl -e bash -c "cd /mnt/c/Users/Adam/Desktop/PAE/Mecalux-AMR/optimality/02_layer_planner && ./build/planner ${algorithmId} ${graphId} ${numTasks} ${numRobots}"`;
  
  try {
    const { stdout, stderr } = await execAsync(command);
    
    if (stderr) {
      throw new Error(stderr);
    }
    
    // Parse the output (you'd need to modify the C++ program to output JSON)
    const result = parseOutput(stdout);
    return result;
    
  } catch (error) {
    throw new Error(`Planner execution failed: ${error instanceof Error ? error.message : String(error)}`);
  }
}

function parseOutput(output: string): PlannerResult {
  // This is a simplified parser - you'd need to implement proper parsing
  // based on your C++ program's output format
  
  const lines = output.split('\n');
  
  // Extract algorithm name
  const algorithmLine = lines.find(line => line.includes('=== Algorithm:'));
  const algorithm = algorithmLine ? algorithmLine.split('===')[1].trim() : 'Unknown';
  
  // Extract makespan
  const makespanLine = lines.find(line => line.includes('makespan') || line.includes('completion time'));
  const makespanMatch = makespanLine?.match(/(\d+\.?\d*)\s*(?:seconds|s)/);
  const makespan = makespanMatch ? parseFloat(makespanMatch[1]) : 0;
  
  // Extract computation time
  const computationLine = lines.find(line => line.includes('computation time'));
  const computationMatch = computationLine?.match(/(\d+\.?\d*)\s*ms/);
  const computationTime = computationMatch ? parseFloat(computationMatch[1]) : 0;
  
  // Parse robot data (simplified - you'd need more sophisticated parsing)
  const robots = parseRobotData(output);
  
  return {
    algorithm,
    makespan,
    computationTime,
    robots
  };
}

function parseRobotData(output: string) {
  // Simplified robot data parsing
  // In reality, you'd need to parse the detailed robot assignment reports
  
  const robots = [];
  const robotSections = output.split(/┌─ Robot \d+ ─/);
  
  for (let i = 1; i < robotSections.length; i++) {
    const section = robotSections[i];
    
    // Extract robot completion time and battery
    const completionMatch = section.match(/Total completion time: (\d+\.?\d*)/);
    const batteryMatch = section.match(/Final battery level:\s+(\d+\.?\d*)/);
    
    const robot = {
      id: i - 1,
      completionTime: completionMatch ? parseFloat(completionMatch[1]) : 0,
      finalBattery: batteryMatch ? parseFloat(batteryMatch[1]) : 0,
      tasks: parseTasksFromRobotSection(section)
    };
    
    robots.push(robot);
  }
  
  return robots;
}

function parseTasksFromRobotSection(section: string) {
  const tasks = [];
  const taskMatches = section.matchAll(/Task ID (\d+):(.*?)(?=Task ID|\★|$)/gs);
  
  for (const match of taskMatches) {
    const taskId = parseInt(match[1]);
    const taskContent = match[2];
    
    // Parse task details
    const fromNodeMatch = taskContent.match(/From: Node (\d+)/);
    const toNodeMatch = taskContent.match(/To:\s+Node (\d+)/);
    const travelMatch = taskContent.match(/Travel to origin: (\d+\.?\d*)s/);
    const executionMatch = taskContent.match(/Task execution:\s+(\d+\.?\d*)s/);
    const cumulativeMatch = taskContent.match(/Cumulative time:\s+(\d+\.?\d*)s/);
    const batteryMatch = taskContent.match(/Battery level:\s+(\d+\.?\d*)%/);
    
    const task = {
      taskId,
      fromNode: fromNodeMatch ? parseInt(fromNodeMatch[1]) : 0,
      toNode: toNodeMatch ? parseInt(toNodeMatch[1]) : 0,
      travelTime: travelMatch ? parseFloat(travelMatch[1]) : 0,
      executionTime: executionMatch ? parseFloat(executionMatch[1]) : 0,
      cumulativeTime: cumulativeMatch ? parseFloat(cumulativeMatch[1]) : 0,
      batteryLevel: batteryMatch ? parseFloat(batteryMatch[1]) : 0,
    };
    
    tasks.push(task);
  }
  
  return tasks;
}

// Express.js route handler example:
/*
app.post('/api/run-planner', async (req, res) => {
  try {
    const result = await runPlanner(req.body);
    res.json(result);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});
*/