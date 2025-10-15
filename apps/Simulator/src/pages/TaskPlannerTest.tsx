import { useState, useEffect } from 'react';

interface PlannerParams {
  algorithmId: number;
  graphId: number;
  numTasks: number;
  numRobots: number;
  mode?: string;
}

interface Task {
  id: string;
  taskId: number;
  fromNode: number | null;
  fromCoords: { x: number; y: number } | null;
  toNode: number | null;
  toCoords: { x: number; y: number } | null;
  travelTime: number;
  executionTime: number;
  cumulativeTime: number;
  batteryLevel: number;
  startTime: number;
  duration: number;
  type?: 'task';
}

interface ChargingEvent {
  type: 'charging';
  batteryBefore: number;
  batteryAfter: number;
  travelTime: number;
  chargingTime: number;
  startTime: number;
  duration: number;
  decision?: {
    currentBattery: number;
    nextTaskId: number;
    nextTaskConsumption: number;
    batteryAfterTask: number;
    threshold: number;
  };
}

type RobotEvent = Task | ChargingEvent;

interface Robot {
  id: number;
  tasks: RobotEvent[];
  initialBattery: number;
  finalBattery: number;
  completionTime: number;
}

interface ComparisonResult {
  algorithm: string;
  makespan: number;
  computationTime: number;
  improvement?: number | null;
}

interface PlannerResult {
  algorithm: string;
  makespan: number;
  computationTime: number;
  robots: Robot[];
  comparison?: ComparisonResult[];
}

export default function TaskPlanner() {
  const [params, setParams] = useState<PlannerParams>({
    algorithmId: 2,
    graphId: 1,
    numTasks: 15,
    numRobots: 2,
  });
  
  const [result, setResult] = useState<PlannerResult | null>(null);
  const [isRunning, setIsRunning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [availableGraphs, setAvailableGraphs] = useState<number[]>([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
  const [loadingGraphs, setLoadingGraphs] = useState(true);

  // Fetch available graphs on component mount
  useEffect(() => {
    const fetchGraphs = async () => {
      try {
        const response = await fetch('/api/graphs');
        if (response.ok) {
          const data = await response.json();
          setAvailableGraphs(data.graphs);
          
          // Ensure current graphId is valid
          if (!data.graphs.includes(params.graphId)) {
            setParams(prev => ({ ...prev, graphId: data.graphs[0] || 1 }));
          }
        }
      } catch (error) {
        console.error('Failed to fetch available graphs:', error);
        // Keep default graphs
      } finally {
        setLoadingGraphs(false);
      }
    };
    
    fetchGraphs();
  }, [params.graphId]);

  const handleRun = async () => {
    setIsRunning(true);
    setError(null);
    setResult(null);

    try {
      // Validate parameters
      if (params.numRobots < 1 || params.numRobots > 10) {
        throw new Error('Number of robots must be between 1 and 10');
      }
      if (params.numTasks < 1) {
        throw new Error('Number of tasks must be at least 1');
      }

      // Add mode for comparison
      const requestParams = {
        ...params,
        mode: params.algorithmId === 0 ? 'comparison' : undefined
      };
      
      console.log('Making API call with params:', requestParams);
      
      const response = await fetch('/api/planner', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(requestParams),
      });

      console.log('Response status:', response.status);
      
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      const data = await response.json();
      console.log('Received data:', data);
      setResult(data);
    } catch (err) {
      console.error('Error:', err);
      setError(err instanceof Error ? err.message : 'Unknown error occurred');
    } finally {
      setIsRunning(false);
    }
  };

  const maxTime = result?.robots && result.robots.length > 0 
    ? Math.max(...result.robots.flatMap(robot => 
        robot.tasks.map(task => task.startTime + task.duration)
      )) 
    : 100;

  return (
    <div className="min-h-screen bg-slate-50">
      <div className="container mx-auto p-6">
        <div className="mb-8">
          <h1 className="text-3xl font-bold text-slate-900 mb-2">Multi-Robot Task Planner</h1>
          <p className="text-slate-600">Optimize task allocation across multiple robots using advanced algorithms</p>
        </div>

        {/* Parameter Configuration */}
        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <h2 className="text-xl font-semibold text-slate-900 mb-4">Configuration</h2>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">Algorithm</label>
              <select 
                value={params.algorithmId}
                onChange={(e) => setParams({...params, algorithmId: parseInt(e.target.value)})}
                className="w-full px-3 py-2 border border-slate-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
              >
                <option value={0}>0 - Comparison Mode</option>
                <option value={1}>1 - Brute Force</option>
                <option value={2}>2 - Greedy</option>
                <option value={3}>3 - Hill Climbing</option>
              </select>
            </div>
            
            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">Graph ID</label>
              <select
                value={params.graphId}
                onChange={(e) => setParams({...params, graphId: parseInt(e.target.value)})}
                className="w-full px-3 py-2 border border-slate-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent bg-white"
                disabled={loadingGraphs}
              >
                {loadingGraphs ? (
                  <option>Loading graphs...</option>
                ) : (
                  availableGraphs.map(id => (
                    <option key={id} value={id}>
                      Graph {id}
                    </option>
                  ))
                )}
              </select>
            </div>
            
            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">Number of Tasks</label>
              <input 
                type="number"
                min="1"
                max="200"
                value={params.numTasks}
                onChange={(e) => setParams({...params, numTasks: Math.max(1, parseInt(e.target.value))})}
                className="w-full px-3 py-2 border border-slate-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
              />
            </div>
            
            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">Number of Robots</label>
              <input 
                type="number"
                min="1"
                max="10"
                value={params.numRobots}
                onChange={(e) => setParams({...params, numRobots: Math.min(10, Math.max(1, parseInt(e.target.value)))})}
                className="w-full px-3 py-2 border border-slate-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
              />
              <p className="text-xs text-slate-500 mt-1">Range: 1-10</p>
            </div>
          </div>
          
          <div className="mt-6 flex items-center justify-between">
            <div className="flex items-center space-x-4">
              <button 
                onClick={handleRun}
                disabled={isRunning}
                className="bg-blue-600 hover:bg-blue-700 disabled:bg-blue-300 text-white px-6 py-2 rounded-lg font-medium transition-colors duration-200 flex items-center space-x-2"
              >
                {isRunning ? (
                  <>
                    <div className="animate-spin rounded-full h-4 w-4 border-2 border-white border-t-transparent"></div>
                    <span>Running...</span>
                  </>
                ) : (
                  <>
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.828 14.828a4 4 0 01-5.656 0M9 10h1m4 0h1m-6 4h1m4 0h1m6-10V9a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4a3 3 0 11-6 0v5a3 3 0 11-6 0V4"/>
                    </svg>
                    <span>Run Planner</span>
                  </>
                )}
              </button>
            </div>
            
            {params.algorithmId === 1 && params.numTasks > 8 && (
              <div className="bg-yellow-100 border border-yellow-400 text-yellow-700 px-3 py-2 rounded text-sm">
                ⚠️ Brute force with {params.numTasks} tasks may take very long. Consider using Greedy or Hill Climbing.
              </div>
            )}
          </div>
        </div>

        {/* Error Display */}
        {error && (
          <div className="bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded mb-6">
            <div className="flex items-center">
              <svg className="w-5 h-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clipRule="evenodd"/>
              </svg>
              <strong>Error:</strong> {error}
            </div>
          </div>
        )}

        {/* Results Display */}
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
            <div className="bg-white rounded-lg shadow-md p-6">
              <h3 className="text-lg font-semibold text-slate-900 mb-4">Execution Timeline</h3>
              <div className="space-y-4">
                {result.robots.map((robot) => (
                  <div key={robot.id} className="flex items-center space-x-4">
                    <div className="w-20 text-sm font-medium text-slate-700">
                      Robot {robot.id}
                    </div>
                    <div className="flex-1 relative h-8 bg-slate-100 rounded">
                      {robot.tasks.map((event, idx) => {
                        const widthPercent = Math.max(2, (event.duration / maxTime) * 100);
                        const leftPercent = Math.min(98 - widthPercent, (event.startTime / maxTime) * 100);
                        
                        if (event.type === 'charging') {
                          return (
                            <div
                              key={`charging-${idx}`}
                              className="absolute h-6 top-1 rounded text-xs text-white flex items-center justify-center font-medium bg-yellow-500 border-2 border-yellow-600"
                              style={{
                                left: `${leftPercent}%`,
                                width: `${widthPercent}%`,
                              }}
                              title={`⚡ CHARGING\nStart: ${event.startTime.toFixed(1)}s\nTravel: ${event.travelTime.toFixed(1)}s\nCharging: ${event.chargingTime.toFixed(1)}s\nBattery: ${event.batteryBefore.toFixed(1)}% → ${event.batteryAfter.toFixed(1)}%`}
                            >
                              ⚡
                            </div>
                          );
                        }
                        
                        return (
                          <div
                            key={event.id}
                            className={`absolute h-6 top-1 rounded text-xs text-white flex items-center justify-center font-medium ${
                              idx % 4 === 0 ? 'bg-blue-500' : 
                              idx % 4 === 1 ? 'bg-green-500' : 
                              idx % 4 === 2 ? 'bg-purple-500' : 'bg-orange-500'
                            }`}
                            style={{
                              left: `${leftPercent}%`,
                              width: `${widthPercent}%`,
                            }}
                            title={`${event.id}: ${event.startTime.toFixed(1)}s - ${(event.startTime + event.duration).toFixed(1)}s (Duration: ${event.duration.toFixed(1)}s)\nFrom Node ${event.fromNode} to Node ${event.toNode}\nBattery: ${event.batteryLevel.toFixed(1)}%`}
                          >
                            {event.id.replace('T', '')}
                          </div>
                        );
                      })}
                    </div>
                    <div className="w-16 text-xs text-slate-500">
                      {robot.tasks.length > 0 && 
                        `${Math.max(...robot.tasks.map(t => t.startTime + t.duration)).toFixed(1)}s`
                      }
                    </div>
                  </div>
                ))}
              </div>
              <div className="mt-4 flex justify-between text-xs text-slate-500">
                <span>0s</span>
                <span>{maxTime.toFixed(1)}s</span>
              </div>
              {result.robots.length > 0 && (
                <div className="mt-2 text-xs text-slate-500">
                  Total tasks: {result.robots.reduce((sum, robot) => sum + robot.tasks.length, 0)}
                </div>
              )}
            </div>

            {/* Robot Assignment Cards - Complete Details */}
            <div className="space-y-4">
              <h3 className="text-lg font-semibold text-slate-900">Detailed Robot Assignments</h3>
              <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
                {result.robots.map((robot) => (
                  <div key={robot.id} className="bg-white rounded-lg shadow-lg border-t-4 border-blue-500 overflow-hidden">
                    {/* Robot Header */}
                    <div className="bg-gradient-to-r from-blue-50 to-blue-100 p-4 border-b border-blue-200">
                      <div className="flex items-center justify-between mb-2">
                        <h4 className="text-xl font-bold text-blue-900">Robot {robot.id}</h4>
                        <span className="bg-blue-600 text-white text-xs font-bold px-3 py-1 rounded-full">
                          {robot.tasks.filter(e => e.type !== 'charging').length} Tasks
                          {robot.tasks.filter(e => e.type === 'charging').length > 0 && 
                            ` + ${robot.tasks.filter(e => e.type === 'charging').length} ⚡`
                          }
                        </span>
                      </div>
                      <div className="grid grid-cols-2 gap-3 text-sm">
                        <div>
                          <span className="text-blue-600 font-medium">Initial Battery:</span>
                          <div className="text-blue-900 font-bold">{robot.initialBattery.toFixed(1)}%</div>
                        </div>
                        <div>
                          <span className="text-blue-600 font-medium">Final Battery:</span>
                          <div className="text-blue-900 font-bold">{robot.finalBattery.toFixed(1)}%</div>
                        </div>
                        <div className="col-span-2">
                          <span className="text-blue-600 font-medium">Total Completion Time:</span>
                          <div className="text-blue-900 font-bold text-lg">{robot.completionTime.toFixed(2)}s</div>
                        </div>
                      </div>
                    </div>
                    
                    {/* Tasks List */}
                    <div className="p-4 space-y-3 max-h-96 overflow-y-auto">
                      {robot.tasks.map((event, idx) => {
                        // Render charging event
                        if (event.type === 'charging') {
                          return (
                            <div key={`charging-${idx}`} className="border-2 border-yellow-500 bg-yellow-50 rounded-lg p-3 hover:shadow-md transition-shadow">
                              <div className="flex items-center justify-between mb-2">
                                <span className="text-sm font-bold text-yellow-900 bg-yellow-200 px-2 py-1 rounded flex items-center gap-1">
                                  ⚡ CHARGING EVENT
                                </span>
                                <span className="text-xs text-yellow-700">
                                  Event {idx + 1} of {robot.tasks.length}
                                </span>
                              </div>
                              
                              {/* Charging Decision */}
                              {event.decision && (
                                <div className="bg-orange-50 border border-orange-200 rounded p-2 mb-2 text-xs space-y-1">
                                  <div className="font-bold text-orange-900 mb-1">⚠️ Preventive Charging Decision</div>
                                  <div className="flex items-center justify-between">
                                    <span className="text-orange-700">Current Battery:</span>
                                    <span className="font-medium text-orange-900">{event.decision.currentBattery.toFixed(1)}%</span>
                                  </div>
                                  <div className="flex items-center justify-between">
                                    <span className="text-orange-700">Next Task (ID {event.decision.nextTaskId}):</span>
                                    <span className="font-medium text-orange-900">-{event.decision.nextTaskConsumption.toFixed(1)}%</span>
                                  </div>
                                  <div className="flex items-center justify-between">
                                    <span className="text-orange-700">Would Result In:</span>
                                    <span className="font-bold text-red-900">{event.decision.batteryAfterTask.toFixed(1)}% (Below {event.decision.threshold.toFixed(0)}%)</span>
                                  </div>
                                </div>
                              )}
                              
                              {/* Charging Details */}
                              <div className="grid grid-cols-2 gap-2 text-xs mb-2">
                                <div className="bg-yellow-100 rounded p-2">
                                  <div className="text-yellow-700 font-medium">Travel to Charger</div>
                                  <div className="text-yellow-900 font-bold">{event.travelTime.toFixed(2)}s</div>
                                </div>
                                <div className="bg-yellow-100 rounded p-2">
                                  <div className="text-yellow-700 font-medium">Charging Time</div>
                                  <div className="text-yellow-900 font-bold">{event.chargingTime.toFixed(2)}s</div>
                                </div>
                              </div>
                              
                              {/* Battery Change */}
                              <div className="grid grid-cols-2 gap-2 text-xs">
                                <div className="bg-red-50 rounded p-2">
                                  <div className="text-red-600 font-medium">Battery on Arrival</div>
                                  <div className="text-red-900 font-bold">{event.batteryBefore.toFixed(1)}%</div>
                                </div>
                                <div className="bg-green-50 rounded p-2">
                                  <div className="text-green-600 font-medium">After Charging</div>
                                  <div className="text-green-900 font-bold">{event.batteryAfter.toFixed(1)}%</div>
                                </div>
                              </div>
                              
                              {/* Total Time */}
                              <div className="mt-2 bg-yellow-100 rounded p-2 text-xs">
                                <div className="text-yellow-700 font-medium">Total Duration</div>
                                <div className="text-yellow-900 font-bold">{event.duration.toFixed(2)}s (Start: {event.startTime.toFixed(1)}s)</div>
                              </div>
                              
                              {idx < robot.tasks.length - 1 && (
                                <div className="mt-2 text-center text-yellow-500 text-xs">↓</div>
                              )}
                            </div>
                          );
                        }
                        
                        // Render normal task
                        const task = event as Task;
                        return (
                          <div key={task.id} className="border border-slate-200 rounded-lg p-3 hover:shadow-md transition-shadow">
                            <div className="flex items-center justify-between mb-2">
                              <span className="text-sm font-bold text-slate-900 bg-slate-100 px-2 py-1 rounded">
                                {task.id}
                              </span>
                              <span className="text-xs text-slate-500">
                                Event {idx + 1} of {robot.tasks.length}
                              </span>
                            </div>
                            
                            {/* Route Information */}
                            <div className="bg-slate-50 rounded p-2 mb-2 text-xs space-y-1">
                              <div className="flex items-center justify-between">
                                <span className="text-slate-600">From:</span>
                                <span className="font-medium text-slate-900">
                                  Node {task.fromNode} ({task.fromCoords?.x.toFixed(1)}, {task.fromCoords?.y.toFixed(1)})
                                </span>
                              </div>
                              <div className="flex items-center justify-between">
                                <span className="text-slate-600">To:</span>
                                <span className="font-medium text-slate-900">
                                  Node {task.toNode} ({task.toCoords?.x.toFixed(1)}, {task.toCoords?.y.toFixed(1)})
                                </span>
                              </div>
                            </div>
                            
                            {/* Timing Information */}
                            <div className="grid grid-cols-2 gap-2 text-xs mb-2">
                              <div className="bg-purple-50 rounded p-2">
                                <div className="text-purple-600 font-medium">Travel Time</div>
                                <div className="text-purple-900 font-bold">{task.travelTime.toFixed(2)}s</div>
                              </div>
                              <div className="bg-green-50 rounded p-2">
                                <div className="text-green-600 font-medium">Execution Time</div>
                                <div className="text-green-900 font-bold">{task.executionTime.toFixed(2)}s</div>
                              </div>
                            </div>
                            
                            {/* Timeline & Battery */}
                            <div className="grid grid-cols-2 gap-2 text-xs">
                              <div className="bg-blue-50 rounded p-2">
                                <div className="text-blue-600 font-medium">Cumulative Time</div>
                                <div className="text-blue-900 font-bold">{task.cumulativeTime.toFixed(2)}s</div>
                              </div>
                              <div className={`rounded p-2 ${
                                task.batteryLevel > 80 ? 'bg-green-50' : 
                                task.batteryLevel > 50 ? 'bg-yellow-50' : 'bg-red-50'
                              }`}>
                                <div className={`font-medium ${
                                  task.batteryLevel > 80 ? 'text-green-600' : 
                                  task.batteryLevel > 50 ? 'text-yellow-600' : 'text-red-600'
                                }`}>Battery After</div>
                                <div className={`font-bold ${
                                  task.batteryLevel > 80 ? 'text-green-900' : 
                                  task.batteryLevel > 50 ? 'text-yellow-900' : 'text-red-900'
                                }`}>{task.batteryLevel.toFixed(1)}%</div>
                              </div>
                            </div>
                            
                            {idx < robot.tasks.length - 1 && (
                              <div className="mt-2 text-center text-slate-400 text-xs">↓</div>
                            )}
                          </div>
                        );
                      })}
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}