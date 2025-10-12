import { useState } from 'react';

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
  // For comparison mode
  comparison?: Array<{
    algorithm: string;
    makespan: number;
    computationTime: number;
    improvement?: number;
  }>;
}

interface PlannerParams {
  algorithmId: number;
  graphId: number;
  numTasks: number;
  numRobots: number;
}

export default function TaskPlanner() {
  const [params, setParams] = useState<PlannerParams>({
    algorithmId: 2, // Default to Greedy
    graphId: 1,
    numTasks: 15,
    numRobots: 2,
  });
  
  const [isRunning, setIsRunning] = useState(false);
  const [result, setResult] = useState<PlannerResult | null>(null);
  const [error, setError] = useState<string | null>(null);

  const algorithms = [
    { id: 0, name: 'Comparison Mode', description: 'Runs all 3 algorithms for comparison' },
    { id: 1, name: 'Brute Force', description: 'Optimal solution (very slow for >8 tasks)' },
    { id: 2, name: 'Greedy', description: 'Fast heuristic approach' },
    { id: 3, name: 'Hill Climbing', description: 'Improved greedy via local search' },
  ];

  const handleRun = async () => {
    setIsRunning(true);
    setError(null);
    setResult(null);

    try {
      // Check if brute force with too many tasks
      if (params.algorithmId === 1 && params.numTasks > 8) {
        throw new Error(`Brute force algorithm limited to ≤8 tasks. You have ${params.numTasks} tasks. Use Greedy (2) or Hill Climbing (3) instead.`);
      }

      // Try to make API call to backend
      try {
        const response = await fetch('/api/planner', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(params),
        });

        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        const data = await response.json();
        setResult(data);
      } catch (fetchError) {
        // If API call fails, use mock data for demo
        console.warn('API not available, using mock data:', fetchError);
        setError(`API connection failed: ${fetchError instanceof Error ? fetchError.message : 'Unknown error'}. Backend server must be running on port 3001.`);
        const mockResult = generateMockResult(params);
        setResult(mockResult);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error occurred');
    } finally {
      setIsRunning(false);
    }
  };

  const generateMockResult = (params: PlannerParams): PlannerResult => {
    const algorithmNames = ['Comparison', 'Brute Force', 'Greedy', 'Hill Climbing'];
    const baseTime = 120 + Math.random() * 100; // Random base completion time
    
    // For comparison mode, generate results for all algorithms
    if (params.algorithmId === 0) {
      const greedyTime = baseTime * 1.1;
      const hillClimbingTime = baseTime * 0.98;
      const bruteForceTime = baseTime * 0.95;
      
      const comparison = [
        {
          algorithm: 'Greedy',
          makespan: greedyTime,
          computationTime: Math.random() * 5 + 0.1,
        },
        {
          algorithm: 'Hill Climbing',
          makespan: hillClimbingTime,
          computationTime: Math.random() * 50 + 5,
          improvement: ((greedyTime - hillClimbingTime) / greedyTime) * 100,
        },
        {
          algorithm: 'Brute Force',
          makespan: bruteForceTime,
          computationTime: Math.random() * 1000 + 100,
          improvement: ((greedyTime - bruteForceTime) / greedyTime) * 100,
        },
      ];

      return {
        algorithm: 'Comparison Mode',
        makespan: Math.min(...comparison.map(c => c.makespan)),
        computationTime: comparison.reduce((sum, c) => sum + c.computationTime, 0),
        robots: [],
        comparison,
      };
    }
    
    // Simulate algorithm performance differences
    let makespan = baseTime;
    let computationTime = 1;
    
    switch (params.algorithmId) {
      case 1: // Brute Force
        makespan *= 0.95; // Slightly better result
        computationTime = Math.random() * 1000 + 100; // Slower
        break;
      case 2: // Greedy
        makespan *= 1.1; // Slightly worse result
        computationTime = Math.random() * 5 + 0.1; // Fast
        break;
      case 3: // Hill Climbing
        makespan *= 0.98; // Good result
        computationTime = Math.random() * 50 + 5; // Medium
        break;
    }

    const robots = [];
    const tasksPerRobot = Math.ceil(params.numTasks / params.numRobots);
    
    for (let robotId = 0; robotId < params.numRobots; robotId++) {
      const robotTasks = [];
      let cumulativeTime = 0;
      let battery = 100;
      
      const numTasksForRobot = Math.min(tasksPerRobot, params.numTasks - robotId * tasksPerRobot);
      
      for (let taskIdx = 0; taskIdx < numTasksForRobot; taskIdx++) {
        const taskId = robotId * tasksPerRobot + taskIdx;
        const travelTime = Math.random() * 15 + 5;
        const executionTime = Math.random() * 10 + 5;
        cumulativeTime += travelTime + executionTime;
        battery -= (travelTime + executionTime) * 0.5;
        
        robotTasks.push({
          taskId,
          fromNode: Math.floor(Math.random() * 6) + 2, // Pickup nodes 2-7
          toNode: Math.floor(Math.random() * 6) + 11, // Dropoff nodes 11-16
          travelTime,
          executionTime,
          cumulativeTime,
          batteryLevel: Math.max(0, battery),
        });
      }
      
      robots.push({
        id: robotId,
        completionTime: cumulativeTime,
        finalBattery: Math.max(0, battery),
        tasks: robotTasks,
      });
    }

    return {
      algorithm: algorithmNames[params.algorithmId] || 'Unknown',
      makespan: Math.max(...robots.map(r => r.completionTime)),
      computationTime,
      robots,
    };
  };

  return (
    <div className="min-h-full p-6 bg-slate-50">
      <div className="mx-auto max-w-6xl space-y-6">
        {/* Header */}
        <header>
          <h1 className="text-3xl font-bold text-slate-900">Multi-Robot Task Planner</h1>
          <p className="text-slate-600 mt-2">
            Configure and run optimization algorithms for multi-robot task allocation in warehouse environments.
          </p>
        </header>

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Configuration Panel */}
          <div className="lg:col-span-1">
            <div className="bg-white rounded-xl shadow-sm border border-slate-200 p-6 space-y-6">
              <h2 className="text-xl font-semibold text-slate-900">Configuration</h2>
              
              {/* Algorithm Selection */}
              <div className="space-y-3">
                <label className="block text-sm font-medium text-slate-700">Algorithm</label>
                <select
                  value={params.algorithmId}
                  onChange={(e) => setParams(prev => ({ ...prev, algorithmId: parseInt(e.target.value) }))}
                  className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                >
                  {algorithms.map(alg => (
                    <option key={alg.id} value={alg.id}>
                      {alg.name}
                    </option>
                  ))}
                </select>
                <p className="text-xs text-slate-500">
                  {algorithms.find(a => a.id === params.algorithmId)?.description}
                </p>
              </div>

              {/* Graph Selection */}
              <div className="space-y-3">
                <label className="block text-sm font-medium text-slate-700">Warehouse Graph</label>
                <select
                  value={params.graphId}
                  onChange={(e) => setParams(prev => ({ ...prev, graphId: parseInt(e.target.value) }))}
                  className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                >
                  {[...Array(10)].map((_, i) => (
                    <option key={i + 1} value={i + 1}>
                      Graph {i + 1}
                    </option>
                  ))}
                </select>
              </div>

              {/* Number of Tasks */}
              <div className="space-y-3">
                <label className="block text-sm font-medium text-slate-700">Number of Tasks</label>
                <input
                  type="number"
                  min="1"
                  max="50"
                  value={params.numTasks}
                  onChange={(e) => setParams(prev => ({ ...prev, numTasks: parseInt(e.target.value) }))}
                  className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                />
                <p className="text-xs text-slate-500">
                  Brute force algorithm limited to ≤8 tasks
                </p>
              </div>

              {/* Number of Robots */}
              <div className="space-y-3">
                <label className="block text-sm font-medium text-slate-700">Number of Robots</label>
                <input
                  type="number"
                  min="1"
                  max="10"
                  value={params.numRobots}
                  onChange={(e) => setParams(prev => ({ ...prev, numRobots: parseInt(e.target.value) }))}
                  className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500"
                />
              </div>

              {/* Run Button */}
              <button
                onClick={handleRun}
                disabled={isRunning}
                className="w-full bg-blue-600 hover:bg-blue-700 disabled:bg-blue-300 text-white font-medium py-3 px-4 rounded-lg transition-colors duration-200 flex items-center justify-center gap-2"
              >
                {isRunning ? (
                  <>
                    <div className="animate-spin rounded-full h-4 w-4 border-2 border-white border-t-transparent"></div>
                    Running...
                  </>
                ) : (
                  'Run Planner'
                )}
              </button>
            </div>
          </div>

          {/* Results Panel */}
          <div className="lg:col-span-2">
            <div className="bg-white rounded-xl shadow-sm border border-slate-200 p-6">
              <h2 className="text-xl font-semibold text-slate-900 mb-6">Results</h2>
              
              {error && (
                <div className="bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded-lg mb-6">
                  <strong>Error:</strong> {error}
                </div>
              )}

              {!result && !error && !isRunning && (
                <div className="text-center py-12 text-slate-500">
                  <div className="text-4xl mb-4">🤖</div>
                  <p>Configure parameters and click "Run Planner" to see optimization results</p>
                </div>
              )}

              {result && (
                <div className="space-y-6">
                  {/* Summary Stats */}
                  <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                    <div className="bg-gradient-to-r from-blue-50 to-blue-100 rounded-lg p-4">
                      <div className="text-blue-600 text-sm font-medium">Algorithm</div>
                      <div className="text-blue-900 text-xl font-bold">{result.algorithm}</div>
                    </div>
                    <div className="bg-gradient-to-r from-green-50 to-green-100 rounded-lg p-4">
                      <div className="text-green-600 text-sm font-medium">Best Makespan</div>
                      <div className="text-green-900 text-xl font-bold">{result.makespan.toFixed(2)}s</div>
                    </div>
                    <div className="bg-gradient-to-r from-purple-50 to-purple-100 rounded-lg p-4">
                      <div className="text-purple-600 text-sm font-medium">Total Computation</div>
                      <div className="text-purple-900 text-xl font-bold">{result.computationTime.toFixed(2)}ms</div>
                    </div>
                  </div>

                  {/* Comparison Results */}
                  {result.comparison && (
                    <div className="space-y-4">
                      <h3 className="text-lg font-medium text-slate-900">Algorithm Comparison</h3>
                      <div className="bg-white border border-slate-200 rounded-lg overflow-hidden">
                        <table className="w-full">
                          <thead className="bg-slate-50">
                            <tr>
                              <th className="px-4 py-3 text-left text-sm font-medium text-slate-900">Algorithm</th>
                              <th className="px-4 py-3 text-left text-sm font-medium text-slate-900">Makespan</th>
                              <th className="px-4 py-3 text-left text-sm font-medium text-slate-900">Computation Time</th>
                              <th className="px-4 py-3 text-left text-sm font-medium text-slate-900">Improvement</th>
                            </tr>
                          </thead>
                          <tbody className="divide-y divide-slate-200">
                            {result.comparison.map((comp, idx) => (
                              <tr key={idx} className={idx === 0 ? 'bg-yellow-50' : ''}>
                                <td className="px-4 py-3 text-sm font-medium text-slate-900">
                                  {comp.algorithm}
                                  {idx === 0 && <span className="ml-2 text-yellow-600 text-xs">(Baseline)</span>}
                                </td>
                                <td className="px-4 py-3 text-sm text-slate-900">{comp.makespan.toFixed(2)}s</td>
                                <td className="px-4 py-3 text-sm text-slate-900">{comp.computationTime.toFixed(2)}ms</td>
                                <td className="px-4 py-3 text-sm">
                                  {comp.improvement ? (
                                    <span className={`font-medium ${comp.improvement > 0 ? 'text-green-600' : 'text-red-600'}`}>
                                      {comp.improvement > 0 ? '+' : ''}{comp.improvement.toFixed(1)}%
                                    </span>
                                  ) : (
                                    <span className="text-slate-400">-</span>
                                  )}
                                </td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    </div>
                  )}

                  {/* Timeline Visualization */}
                  <div className="space-y-4">
                    <h3 className="text-lg font-medium text-slate-900">Execution Timeline</h3>
                    <div className="bg-slate-50 rounded-lg p-4">
                      <div className="space-y-3">
                        {result.robots.map((robot) => (
                          <div key={robot.id} className="space-y-2">
                            <div className="flex justify-between items-center text-sm">
                              <span className="font-medium text-slate-900">Robot {robot.id}</span>
                              <span className="text-slate-600">{robot.completionTime.toFixed(1)}s</span>
                            </div>
                            <div className="relative">
                              <div className="h-6 bg-white rounded border overflow-hidden">
                                <div 
                                  className="h-full bg-gradient-to-r from-blue-400 to-blue-600 flex items-center justify-center text-white text-xs font-medium"
                                  style={{ 
                                    width: `${(robot.completionTime / result.makespan) * 100}%` 
                                  }}
                                >
                                  {robot.tasks.length} tasks
                                </div>
                              </div>
                              {/* Battery indicator */}
                              <div className="absolute right-0 top-0 h-6 w-16 bg-gray-200 rounded-r border-l border-slate-300 flex items-center justify-center">
                                <div 
                                  className={`text-xs font-medium ${
                                    robot.finalBattery > 50 ? 'text-green-600' : 
                                    robot.finalBattery > 20 ? 'text-yellow-600' : 'text-red-600'
                                  }`}
                                >
                                  {robot.finalBattery.toFixed(0)}%
                                </div>
                              </div>
                            </div>
                          </div>
                        ))}
                      </div>
                    </div>
                  </div>

                  {/* Robot Assignments */}
                  <div className="space-y-4">
                    <h3 className="text-lg font-medium text-slate-900">Detailed Assignments</h3>
                    {result.robots.map((robot) => (
                      <div key={robot.id} className="border border-slate-200 rounded-lg overflow-hidden">
                        <div className="bg-gradient-to-r from-slate-50 to-slate-100 px-4 py-3 border-b border-slate-200">
                          <div className="flex justify-between items-center">
                            <h4 className="font-medium text-slate-900 flex items-center gap-2">
                              🤖 Robot {robot.id}
                            </h4>
                            <div className="text-sm text-slate-600 flex items-center gap-4">
                              <span>⏱️ {robot.completionTime.toFixed(2)}s</span>
                              <span 
                                className={`font-medium ${
                                  robot.finalBattery > 50 ? 'text-green-600' : 
                                  robot.finalBattery > 20 ? 'text-yellow-600' : 'text-red-600'
                                }`}
                              >
                                🔋 {robot.finalBattery.toFixed(1)}%
                              </span>
                            </div>
                          </div>
                        </div>
                        <div className="p-4">
                          <div className="space-y-2">
                            {robot.tasks.map((task, idx) => (
                              <div key={idx} className="flex items-center justify-between py-3 px-4 bg-gradient-to-r from-blue-50 to-indigo-50 rounded-lg border border-blue-100">
                                <div className="flex items-center gap-3">
                                  <div className="w-8 h-8 bg-blue-500 text-white rounded-full flex items-center justify-center text-sm font-bold">
                                    {task.taskId}
                                  </div>
                                  <div className="text-sm">
                                    <div className="font-medium text-slate-900">
                                      Node {task.fromNode} → Node {task.toNode}
                                    </div>
                                    <div className="text-slate-600 text-xs">
                                      Travel: {task.travelTime.toFixed(1)}s | Execute: {task.executionTime.toFixed(1)}s
                                    </div>
                                  </div>
                                </div>
                                <div className="text-right">
                                  <div className="text-sm font-medium text-slate-900">
                                    {task.cumulativeTime.toFixed(1)}s
                                  </div>
                                  <div className={`text-xs ${
                                    task.batteryLevel > 50 ? 'text-green-600' : 
                                    task.batteryLevel > 20 ? 'text-yellow-600' : 'text-red-600'
                                  }`}>
                                    {task.batteryLevel.toFixed(1)}%
                                  </div>
                                </div>
                              </div>
                            ))}
                          </div>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}