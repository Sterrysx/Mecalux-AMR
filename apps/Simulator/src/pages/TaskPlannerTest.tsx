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
  startTime: number;
  duration: number;
  description: string;
}

interface Robot {
  id: number;
  tasks: Task[];
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
                      {robot.tasks.map((task, idx) => {
                        const widthPercent = Math.max(2, (task.duration / maxTime) * 100);
                        const leftPercent = Math.min(98 - widthPercent, (task.startTime / maxTime) * 100);
                        return (
                          <div
                            key={task.id}
                            className={`absolute h-6 top-1 rounded text-xs text-white flex items-center justify-center font-medium ${
                              idx % 4 === 0 ? 'bg-blue-500' : 
                              idx % 4 === 1 ? 'bg-green-500' : 
                              idx % 4 === 2 ? 'bg-purple-500' : 'bg-orange-500'
                            }`}
                            style={{
                              left: `${leftPercent}%`,
                              width: `${widthPercent}%`,
                            }}
                            title={`${task.description}: ${task.startTime.toFixed(1)}s - ${(task.startTime + task.duration).toFixed(1)}s (Duration: ${task.duration.toFixed(1)}s)`}
                          >
                            {task.id.replace('T', '')}
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

            {/* Robot Assignment Cards */}
            <div className="space-y-4">
              <h3 className="text-lg font-semibold text-slate-900">Detailed Robot Assignments</h3>
              <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                {result.robots.map((robot) => (
                  <div key={robot.id} className="bg-white rounded-lg shadow-md p-4 border-l-4 border-blue-500">
                    <div className="flex items-center justify-between mb-3">
                      <h4 className="text-lg font-semibold text-slate-900">Robot {robot.id}</h4>
                      <span className="bg-blue-100 text-blue-800 text-xs font-medium px-2.5 py-0.5 rounded">
                        {robot.tasks.length} tasks
                      </span>
                    </div>
                    <div className="space-y-2">
                      {robot.tasks.map((task) => (
                        <div key={task.id} className="flex items-center justify-between p-2 bg-slate-50 rounded">
                          <div className="flex items-center space-x-2">
                            <span className="text-sm font-medium text-slate-700">{task.id}</span>
                            <span className="text-xs text-slate-500">{task.description}</span>
                          </div>
                          <div className="text-xs text-slate-500">
                            {task.startTime.toFixed(1)}s - {(task.startTime + task.duration).toFixed(1)}s
                          </div>
                        </div>
                      ))}
                    </div>
                    {robot.tasks.length > 0 && (
                      <div className="mt-3 pt-3 border-t border-slate-200">
                        <div className="text-sm text-slate-600">
                          <strong>Completion:</strong> {Math.max(...robot.tasks.map(t => t.startTime + t.duration)).toFixed(1)}s
                        </div>
                      </div>
                    )}
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