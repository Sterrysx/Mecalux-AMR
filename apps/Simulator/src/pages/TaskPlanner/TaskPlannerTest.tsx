import { useState, useEffect } from 'react';
import TimelineVisualizer from './components/TimelineVisualizer';
import { useTheme } from '../../contexts/ThemeContext';

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
  const { darkMode } = useTheme();
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
  const [showVisualizer, setShowVisualizer] = useState(false);
  const [compactView, setCompactView] = useState(false);

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

  // Set up arrow button event listeners for static timeline
  useEffect(() => {
    if (!result) return;

    const scrollStates: { [key: string]: { isScrolling: boolean; direction: number; animationId?: number; timeoutId?: number } } = {};
    const handlers: { [key: string]: { down: any; up: any } } = {};
    
    // Small delay to ensure DOM is ready
    const setupTimeout = setTimeout(() => {
      result.robots.forEach(robot => {
        const leftButton = document.getElementById(`static-left-arrow-${robot.id}`);
        const rightButton = document.getElementById(`static-right-arrow-${robot.id}`);
        const scrollContainer = document.getElementById(`static-timeline-scroll-${robot.id}`);

        if (leftButton && scrollContainer) {
          const handleLeftDown = (e: Event) => {
            e.preventDefault();
            
            const key = `left-${robot.id}`;
            
            // Wait 200ms - if still pressed, start continuous scroll
            scrollStates[key] = { isScrolling: false, direction: -1 };
            scrollStates[key].timeoutId = window.setTimeout(() => {
              scrollStates[key].isScrolling = true;
              
              const scroll = () => {
                if (scrollStates[key]?.isScrolling) {
                  scrollContainer.scrollLeft += scrollStates[key].direction * 5;
                  scrollStates[key].animationId = requestAnimationFrame(scroll);
                }
              };
              
              scroll();
            }, 200);
          };

          const handleLeftUp = () => {
            const key = `left-${robot.id}`;
            if (scrollStates[key]) {
              const wasHolding = scrollStates[key].isScrolling;
              
              // Clear timeout and animation
              if (scrollStates[key].timeoutId) {
                clearTimeout(scrollStates[key].timeoutId);
              }
              if (scrollStates[key].animationId) {
                cancelAnimationFrame(scrollStates[key].animationId!);
              }
              
              // If it was just a click (not holding), do smooth scroll
              if (!wasHolding) {
                scrollContainer.scrollBy({ left: -200, behavior: 'smooth' });
              } else {
                // If holding, add a smooth deceleration
                let velocity = scrollStates[key].direction * 5;
                const decelerate = () => {
                  velocity *= 0.9; // Deceleration factor
                  if (Math.abs(velocity) > 0.1) {
                    scrollContainer.scrollLeft += velocity;
                    requestAnimationFrame(decelerate);
                  }
                };
                decelerate();
              }
              
              delete scrollStates[key];
            }
          };

          handlers[`left-${robot.id}`] = { down: handleLeftDown, up: handleLeftUp };

          leftButton.addEventListener('pointerdown', handleLeftDown);
          leftButton.addEventListener('pointerup', handleLeftUp);
          leftButton.addEventListener('pointerleave', handleLeftUp);
          leftButton.addEventListener('pointercancel', handleLeftUp);
        }

        if (rightButton && scrollContainer) {
          const handleRightDown = (e: Event) => {
            e.preventDefault();
            
            const key = `right-${robot.id}`;
            
            // Wait 200ms - if still pressed, start continuous scroll
            scrollStates[key] = { isScrolling: false, direction: 1 };
            scrollStates[key].timeoutId = window.setTimeout(() => {
              scrollStates[key].isScrolling = true;
              
              const scroll = () => {
                if (scrollStates[key]?.isScrolling) {
                  scrollContainer.scrollLeft += scrollStates[key].direction * 5;
                  scrollStates[key].animationId = requestAnimationFrame(scroll);
                }
              };
              
              scroll();
            }, 200);
          };

          const handleRightUp = () => {
            const key = `right-${robot.id}`;
            if (scrollStates[key]) {
              const wasHolding = scrollStates[key].isScrolling;
              
              // Clear timeout and animation
              if (scrollStates[key].timeoutId) {
                clearTimeout(scrollStates[key].timeoutId);
              }
              if (scrollStates[key].animationId) {
                cancelAnimationFrame(scrollStates[key].animationId!);
              }
              
              // If it was just a click (not holding), do smooth scroll
              if (!wasHolding) {
                scrollContainer.scrollBy({ left: 200, behavior: 'smooth' });
              } else {
                // If holding, add a smooth deceleration
                let velocity = scrollStates[key].direction * 5;
                const decelerate = () => {
                  velocity *= 0.9; // Deceleration factor
                  if (Math.abs(velocity) > 0.1) {
                    scrollContainer.scrollLeft += velocity;
                    requestAnimationFrame(decelerate);
                  }
                };
                decelerate();
              }
              
              delete scrollStates[key];
            }
          };

          handlers[`right-${robot.id}`] = { down: handleRightDown, up: handleRightUp };

          rightButton.addEventListener('pointerdown', handleRightDown);
          rightButton.addEventListener('pointerup', handleRightUp);
          rightButton.addEventListener('pointerleave', handleRightUp);
          rightButton.addEventListener('pointercancel', handleRightUp);
        }
      });
    }, 50);

    // Cleanup
    return () => {
      clearTimeout(setupTimeout);
      
      // Clean up scroll states
      Object.values(scrollStates).forEach(state => {
        state.isScrolling = false;
        if (state.timeoutId) {
          clearTimeout(state.timeoutId);
        }
        if (state.animationId) {
          cancelAnimationFrame(state.animationId);
        }
      });

      // Remove all event listeners
      result.robots.forEach(robot => {
        const leftButton = document.getElementById(`static-left-arrow-${robot.id}`);
        const rightButton = document.getElementById(`static-right-arrow-${robot.id}`);
        
        const leftKey = `left-${robot.id}`;
        const rightKey = `right-${robot.id}`;
        
        if (leftButton && handlers[leftKey]) {
          leftButton.removeEventListener('pointerdown', handlers[leftKey].down);
          leftButton.removeEventListener('pointerup', handlers[leftKey].up);
          leftButton.removeEventListener('pointerleave', handlers[leftKey].up);
          leftButton.removeEventListener('pointercancel', handlers[leftKey].up);
        }
        
        if (rightButton && handlers[rightKey]) {
          rightButton.removeEventListener('pointerdown', handlers[rightKey].down);
          rightButton.removeEventListener('pointerup', handlers[rightKey].up);
          rightButton.removeEventListener('pointerleave', handlers[rightKey].up);
          rightButton.removeEventListener('pointercancel', handlers[rightKey].up);
        }
      });
    };
  }, [result, showVisualizer, compactView]);

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
    <div className="container mx-auto p-6">
      {/* Header */}
      <div className="mb-8">
        <h1 className={`text-3xl font-bold mb-2 ${darkMode ? 'text-white' : 'text-slate-900'}`}>Multi-Robot Task Planner</h1>
        <p className={darkMode ? 'text-slate-400' : 'text-slate-600'}>Optimize task allocation across multiple robots using advanced algorithms</p>
      </div>

      {/* Parameter Configuration */}
      <div className={`rounded-lg shadow-md p-6 mb-6 ${darkMode ? 'bg-slate-800' : 'bg-white'}`}>
          <h2 className={`text-xl font-semibold mb-4 ${darkMode ? 'text-white' : 'text-slate-900'}`}>Configuration</h2>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
            <div>
              <label className={`block text-sm font-medium mb-2 ${darkMode ? 'text-slate-300' : 'text-slate-700'}`}>Algorithm</label>
              <select 
                value={params.algorithmId}
                onChange={(e) => setParams({...params, algorithmId: parseInt(e.target.value)})}
                className={`w-full px-3 py-2 border rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent ${
                  darkMode ? 'bg-slate-700 border-slate-600 text-white' : 'bg-white border-slate-300 text-slate-900'
                }`}
              >
                <option value={0}>0 - Comparison Mode</option>
                <option value={1}>1 - Brute Force</option>
                <option value={2}>2 - Greedy</option>
                <option value={3}>3 - Hill Climbing</option>
              </select>
            </div>
            
            <div>
              <label className={`block text-sm font-medium mb-2 ${darkMode ? 'text-slate-300' : 'text-slate-700'}`}>Graph ID</label>
              <select
                value={params.graphId}
                onChange={(e) => setParams({...params, graphId: parseInt(e.target.value)})}
                className={`w-full px-3 py-2 border rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent ${
                  darkMode ? 'bg-slate-700 border-slate-600 text-white' : 'bg-white border-slate-300 text-slate-900'
                }`}
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
              <label className={`block text-sm font-medium mb-2 ${darkMode ? 'text-slate-300' : 'text-slate-700'}`}>Number of Tasks</label>
              <input 
                type="number"
                min="1"
                max="200"
                value={params.numTasks}
                onChange={(e) => setParams({...params, numTasks: Math.max(1, parseInt(e.target.value))})}
                className={`w-full px-3 py-2 border rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent ${
                  darkMode ? 'bg-slate-700 border-slate-600 text-white' : 'bg-white border-slate-300 text-slate-900'
                }`}
              />
            </div>
            
            <div>
              <label className={`block text-sm font-medium mb-2 ${darkMode ? 'text-slate-300' : 'text-slate-700'}`}>Number of Robots</label>
              <input 
                type="number"
                min="1"
                max="10"
                value={params.numRobots}
                onChange={(e) => setParams({...params, numRobots: Math.min(10, Math.max(1, parseInt(e.target.value)))})}
                className={`w-full px-3 py-2 border rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent ${
                  darkMode ? 'bg-slate-700 border-slate-600 text-white' : 'bg-white border-slate-300 text-slate-900'
                }`}
              />
              <p className={`text-xs mt-1 ${darkMode ? 'text-slate-400' : 'text-slate-500'}`}>Range: 1-10</p>
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
              <div className={`border px-3 py-2 rounded text-sm ${
                darkMode 
                  ? 'bg-yellow-900 border-yellow-700 text-yellow-300' 
                  : 'bg-yellow-100 border-yellow-400 text-yellow-700'
              }`}>
                ⚠️ Brute force with {params.numTasks} tasks may take very long. Consider using Greedy or Hill Climbing.
              </div>
            )}
          </div>
        </div>

        {/* Error Display */}
        {error && (
          <div className={`border px-4 py-3 rounded mb-6 ${
            darkMode 
              ? 'bg-red-900 border-red-700 text-red-300' 
              : 'bg-red-100 border-red-400 text-red-700'
          }`}>
            <div className="flex items-center">
              <svg className="w-5 h-5 mr-2" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clipRule="evenodd"/>
              </svg>
              <strong>Error:</strong> {error}
            </div>
          </div>
        )}

        {/* Results Display */}
        {result && !showVisualizer && (
          <div className="space-y-6">
            {/* Summary Stats */}
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
              <div className={`rounded-lg p-4 ${
                darkMode 
                  ? 'bg-gradient-to-r from-blue-900 to-blue-800' 
                  : 'bg-gradient-to-r from-blue-50 to-blue-100'
              }`}>
                <div className={`text-sm font-medium ${darkMode ? 'text-blue-300' : 'text-blue-600'}`}>Algorithm</div>
                <div className={`text-xl font-bold ${darkMode ? 'text-blue-100' : 'text-blue-900'}`}>{result.algorithm}</div>
              </div>
              <div className={`rounded-lg p-4 ${
                darkMode 
                  ? 'bg-gradient-to-r from-green-900 to-green-800' 
                  : 'bg-gradient-to-r from-green-50 to-green-100'
              }`}>
                <div className={`text-sm font-medium ${darkMode ? 'text-green-300' : 'text-green-600'}`}>Best Makespan</div>
                <div className={`text-xl font-bold ${darkMode ? 'text-green-100' : 'text-green-900'}`}>{result.makespan.toFixed(2)}s</div>
              </div>
              <div className={`rounded-lg p-4 ${
                darkMode 
                  ? 'bg-gradient-to-r from-purple-900 to-purple-800' 
                  : 'bg-gradient-to-r from-purple-50 to-purple-100'
              }`}>
                <div className={`text-sm font-medium ${darkMode ? 'text-purple-300' : 'text-purple-600'}`}>Total Computation</div>
                <div className={`text-xl font-bold ${darkMode ? 'text-purple-100' : 'text-purple-900'}`}>{result.computationTime.toFixed(2)}ms</div>
              </div>
            </div>

            {/* Timeline Visualizer Button */}
            <div className="flex justify-center">
              <button
                onClick={() => setShowVisualizer(true)}
                className="bg-gradient-to-r from-indigo-600 to-purple-600 text-white px-8 py-4 rounded-lg font-bold text-lg shadow-lg hover:shadow-xl transform hover:scale-105 transition-all duration-200 flex items-center gap-3"
              >
                <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" />
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
                🎬 Launch Timeline Visualizer
              </button>
            </div>

            {/* Comparison Results */}
            {result.comparison && (
              <div className="space-y-4">
                <h3 className={`text-lg font-medium ${darkMode ? 'text-white' : 'text-slate-900'}`}>Algorithm Comparison</h3>
                <div className={`border rounded-lg overflow-hidden ${
                  darkMode ? 'bg-slate-800 border-slate-700' : 'bg-white border-slate-200'
                }`}>
                  <table className="w-full">
                    <thead className={darkMode ? 'bg-slate-700' : 'bg-slate-50'}>
                      <tr>
                        <th className={`px-4 py-3 text-left text-sm font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>Algorithm</th>
                        <th className={`px-4 py-3 text-left text-sm font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>Makespan</th>
                        <th className={`px-4 py-3 text-left text-sm font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>Computation Time</th>
                        <th className={`px-4 py-3 text-left text-sm font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>Improvement</th>
                      </tr>
                    </thead>
                    <tbody className={`divide-y ${darkMode ? 'divide-slate-700' : 'divide-slate-200'}`}>
                      {result.comparison.map((comp, idx) => (
                        <tr key={idx} className={idx === 0 ? (darkMode ? 'bg-yellow-900/30' : 'bg-yellow-50') : ''}>
                          <td className={`px-4 py-3 text-sm font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>
                            {comp.algorithm}
                            {idx === 0 && <span className={`ml-2 text-xs ${darkMode ? 'text-yellow-400' : 'text-yellow-600'}`}>(Baseline)</span>}
                          </td>
                          <td className={`px-4 py-3 text-sm ${darkMode ? 'text-slate-300' : 'text-slate-900'}`}>{comp.makespan.toFixed(2)}s</td>
                          <td className={`px-4 py-3 text-sm ${darkMode ? 'text-slate-300' : 'text-slate-900'}`}>{comp.computationTime.toFixed(2)}ms</td>
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
            <div className={`rounded-lg shadow-md p-6 overflow-hidden ${darkMode ? 'bg-slate-800' : 'bg-white'}`}>
              <div className="flex items-center justify-between mb-4">
                <h3 className={`text-lg font-semibold ${darkMode ? 'text-white' : 'text-slate-900'}`}>Execution Timeline</h3>
                <button
                  onClick={() => setCompactView(!compactView)}
                  className={`flex items-center gap-2 px-3 py-2 rounded-lg font-medium transition-all ${
                    compactView 
                      ? (darkMode ? 'bg-indigo-900 text-indigo-300 hover:bg-indigo-800' : 'bg-indigo-100 text-indigo-700 hover:bg-indigo-200')
                      : (darkMode ? 'bg-slate-700 text-slate-300 hover:bg-slate-600' : 'bg-slate-100 text-slate-700 hover:bg-slate-200')
                  }`}
                  title={compactView ? "Switch to scrollable view" : "Switch to compact view"}
                >
                  <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    {compactView ? (
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4" />
                    ) : (
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4" />
                    )}
                  </svg>
                  {compactView ? 'Expanded' : 'Compact'}
                </button>
              </div>
              <div className="space-y-4">
                {result.robots.map((robot) => (
                  <div key={robot.id} className="flex items-center space-x-4 min-w-0">
                    <div className="w-20 flex-shrink-0 text-sm font-medium text-slate-700">
                      Robot {robot.id}
                    </div>
                    
                    {!compactView ? (
                      <div className="flex-1 min-w-0 flex items-center gap-2">
                        {/* Left Arrow */}
                        <button
                          id={`static-left-arrow-${robot.id}`}
                          className="flex-shrink-0 bg-slate-200 hover:bg-slate-300 active:bg-slate-400 text-slate-700 rounded p-1 transition-colors select-none"
                          style={{ touchAction: 'none' }}
                          title="Scroll left (hold to scroll continuously)"
                        >
                          <svg className="w-4 h-4 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
                          </svg>
                        </button>

                        {/* Timeline */}
                        <div className={`flex-1 min-w-0 relative h-8 rounded overflow-hidden ${
                          darkMode ? 'bg-slate-700' : 'bg-slate-100'
                        }`}>
                          <div 
                            className="relative h-full overflow-x-scroll scrollbar-hide"
                            id={`static-timeline-scroll-${robot.id}`}
                          >
                            <div className="relative h-full" style={{ width: `${Math.max(100, maxTime * 10)}px` }}>
                            {robot.tasks.map((event, idx) => {
                              const widthPx = Math.max(20, event.duration * 10);
                              const leftPx = event.startTime * 10;
                              
                              if (event.type === 'charging') {
                                // Calculate charging-only block (excluding travel time)
                                const chargingOnlyWidth = Math.max(20, event.chargingTime * 10);
                                const chargingStartPx = (event.startTime + event.travelTime) * 10;
                                
                                return (
                                  <div
                                    key={`charging-${idx}`}
                                    className="absolute h-6 top-1 rounded text-xs text-white flex items-center justify-center font-medium bg-yellow-500 border-2 border-yellow-600"
                                    style={{
                                      left: `${chargingStartPx}px`,
                                      width: `${chargingOnlyWidth}px`,
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
                                    left: `${leftPx}px`,
                                    width: `${widthPx}px`,
                                  }}
                                  title={`${event.id}: ${event.startTime.toFixed(1)}s - ${(event.startTime + event.duration).toFixed(1)}s (Duration: ${event.duration.toFixed(1)}s)\nFrom Node ${event.fromNode} to Node ${event.toNode}\nBattery: ${event.batteryLevel.toFixed(1)}%`}
                                >
                                  {event.id.replace('T', '')}
                                </div>
                              );
                            })}
                          </div>
                        </div>
                      </div>

                      {/* Right Arrow */}
                      <button
                        id={`static-right-arrow-${robot.id}`}
                        className="flex-shrink-0 bg-slate-200 hover:bg-slate-300 active:bg-slate-400 text-slate-700 rounded p-1 transition-colors select-none"
                        style={{ touchAction: 'none' }}
                        title="Scroll right (hold to scroll continuously)"
                      >
                        <svg className="w-4 h-4 pointer-events-none" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
                        </svg>
                      </button>
                    </div>
                    ) : (
                      /* Compact View */
                      <div className={`flex-1 min-w-0 relative h-8 rounded overflow-hidden ${
                        darkMode ? 'bg-slate-700' : 'bg-slate-100'
                      }`}>
                        <div className="relative h-full w-full">
                          {robot.tasks.map((event, idx) => {
                            const totalDuration = maxTime;
                            const widthPercent = (event.duration / totalDuration) * 100;
                            const leftPercent = (event.startTime / totalDuration) * 100;
                            
                            if (event.type === 'charging') {
                              const chargingWidthPercent = (event.chargingTime / totalDuration) * 100;
                              const chargingLeftPercent = ((event.startTime + event.travelTime) / totalDuration) * 100;
                              
                              return (
                                <div
                                  key={`charging-${idx}`}
                                  className="absolute h-full rounded overflow-hidden border-2 border-yellow-600"
                                  style={{
                                    left: `${chargingLeftPercent}%`,
                                    width: `${chargingWidthPercent}%`,
                                  }}
                                >
                                  <div className="h-full bg-yellow-500 flex items-center justify-center text-xs text-white font-bold">
                                    ⚡
                                  </div>
                                </div>
                              );
                            }
                            
                            return (
                              <div
                                key={event.id}
                                className="absolute h-full rounded overflow-hidden"
                                style={{
                                  left: `${leftPercent}%`,
                                  width: `${widthPercent}%`,
                                }}
                              >
                                <div
                                  className={`h-full flex items-center justify-center text-xs text-white font-bold ${
                                    idx % 4 === 0 ? 'bg-blue-500' : 
                                    idx % 4 === 1 ? 'bg-green-500' : 
                                    idx % 4 === 2 ? 'bg-purple-500' : 'bg-orange-500'
                                  }`}
                                  title={`${event.id}: ${event.startTime.toFixed(1)}s - ${(event.startTime + event.duration).toFixed(1)}s (Duration: ${event.duration.toFixed(1)}s)\nFrom Node ${event.fromNode} to Node ${event.toNode}\nBattery: ${event.batteryLevel.toFixed(1)}%`}
                                >
                                  {event.id.replace('T', '')}
                                </div>
                              </div>
                            );
                          })}
                        </div>
                      </div>
                    )}
                    
                    <div className="w-16 flex-shrink-0 text-xs text-slate-500">
                      {robot.tasks.length > 0 && 
                        `${Math.max(...robot.tasks.map(t => t.startTime + t.duration)).toFixed(1)}s`
                      }
                    </div>
                  </div>
                ))}
              </div>
              <div className={`mt-4 flex justify-between text-xs ${darkMode ? 'text-slate-400' : 'text-slate-500'}`}>
                <span>0s</span>
                <span>{maxTime.toFixed(1)}s</span>
              </div>
              {result.robots.length > 0 && (
                <div className={`mt-2 text-xs ${darkMode ? 'text-slate-400' : 'text-slate-500'}`}>
                  Total tasks: {result.robots.reduce((sum, robot) => sum + robot.tasks.length, 0)}
                </div>
              )}
            </div>

            {/* Robot Assignment Cards - Complete Details */}
            <div className="space-y-4">
              <h3 className={`text-lg font-semibold ${darkMode ? 'text-white' : 'text-slate-900'}`}>Detailed Robot Assignments</h3>
              <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
                {result.robots.map((robot) => (
                  <div key={robot.id} className={`rounded-lg shadow-lg border-t-4 border-blue-500 overflow-hidden ${
                    darkMode ? 'bg-slate-800' : 'bg-white'
                  }`}>
                    {/* Robot Header */}
                    <div className={`p-4 border-b ${
                      darkMode 
                        ? 'bg-blue-900/30 border-blue-800' 
                        : 'bg-gradient-to-r from-blue-50 to-blue-100 border-blue-200'
                    }`}>
                      <div className="flex items-center justify-between mb-2">
                        <h4 className={`text-xl font-bold ${darkMode ? 'text-blue-300' : 'text-blue-900'}`}>Robot {robot.id}</h4>
                        <span className="bg-blue-600 text-white text-xs font-bold px-3 py-1 rounded-full">
                          {robot.tasks.filter(e => e.type !== 'charging').length} Tasks
                          {robot.tasks.filter(e => e.type === 'charging').length > 0 && 
                            ` + ${robot.tasks.filter(e => e.type === 'charging').length} ⚡`
                          }
                        </span>
                      </div>
                      <div className="grid grid-cols-2 gap-3 text-sm">
                        <div>
                          <span className={`font-medium ${darkMode ? 'text-blue-400' : 'text-blue-600'}`}>Initial Battery:</span>
                          <div className={`font-bold ${darkMode ? 'text-blue-200' : 'text-blue-900'}`}>{robot.initialBattery.toFixed(1)}%</div>
                        </div>
                        <div>
                          <span className={`font-medium ${darkMode ? 'text-blue-400' : 'text-blue-600'}`}>Final Battery:</span>
                          <div className={`font-bold ${darkMode ? 'text-blue-200' : 'text-blue-900'}`}>{robot.finalBattery.toFixed(1)}%</div>
                        </div>
                        <div className="col-span-2">
                          <span className={`font-medium ${darkMode ? 'text-blue-400' : 'text-blue-600'}`}>Total Completion Time:</span>
                          <div className={`font-bold text-lg ${darkMode ? 'text-blue-200' : 'text-blue-900'}`}>{robot.completionTime.toFixed(2)}s</div>
                        </div>
                      </div>
                    </div>
                    
                    {/* Tasks List */}
                    <div className={`p-4 space-y-3 max-h-96 overflow-y-auto ${darkMode ? 'bg-slate-800' : ''}`}>
                      {robot.tasks.map((event, idx) => {
                        // Render charging event
                        if (event.type === 'charging') {
                          return (
                            <div key={`charging-${idx}`} className={`border-2 rounded-lg p-3 hover:shadow-md transition-shadow ${
                              darkMode 
                                ? 'border-yellow-600 bg-yellow-900/20' 
                                : 'border-yellow-500 bg-yellow-50'
                            }`}>
                              <div className="flex items-center justify-between mb-2">
                                <span className={`text-sm font-bold px-2 py-1 rounded flex items-center gap-1 ${
                                  darkMode 
                                    ? 'text-yellow-300 bg-yellow-900/50' 
                                    : 'text-yellow-900 bg-yellow-200'
                                }`}>
                                  ⚡ CHARGING EVENT
                                </span>
                                <span className={`text-xs ${darkMode ? 'text-yellow-400' : 'text-yellow-700'}`}>
                                  Event {idx + 1} of {robot.tasks.length}
                                </span>
                              </div>
                              
                              {/* Charging Decision */}
                              {event.decision && (
                                <div className={`border rounded p-2 mb-2 text-xs space-y-1 ${
                                  darkMode 
                                    ? 'bg-orange-900/30 border-orange-700' 
                                    : 'bg-orange-50 border-orange-200'
                                }`}>
                                  <div className={`font-bold mb-1 ${darkMode ? 'text-orange-300' : 'text-orange-900'}`}>⚠️ Preventive Charging Decision</div>
                                  <div className="flex items-center justify-between">
                                    <span className={darkMode ? 'text-orange-400' : 'text-orange-700'}>Current Battery:</span>
                                    <span className={`font-medium ${darkMode ? 'text-orange-200' : 'text-orange-900'}`}>{event.decision.currentBattery.toFixed(1)}%</span>
                                  </div>
                                  <div className="flex items-center justify-between">
                                    <span className={darkMode ? 'text-orange-400' : 'text-orange-700'}>Next Task (ID {event.decision.nextTaskId}):</span>
                                    <span className={`font-medium ${darkMode ? 'text-orange-200' : 'text-orange-900'}`}>-{event.decision.nextTaskConsumption.toFixed(1)}%</span>
                                  </div>
                                  <div className="flex items-center justify-between">
                                    <span className={darkMode ? 'text-orange-400' : 'text-orange-700'}>Would Result In:</span>
                                    <span className={`font-bold ${darkMode ? 'text-red-300' : 'text-red-900'}`}>{event.decision.batteryAfterTask.toFixed(1)}% (Below {event.decision.threshold.toFixed(0)}%)</span>
                                  </div>
                                </div>
                              )}
                              
                              {/* Charging Details */}
                              <div className="grid grid-cols-2 gap-2 text-xs mb-2">
                                <div className={`rounded p-2 ${darkMode ? 'bg-yellow-900/40' : 'bg-yellow-100'}`}>
                                  <div className={`font-medium ${darkMode ? 'text-yellow-400' : 'text-yellow-700'}`}>Travel to Charger</div>
                                  <div className={`font-bold ${darkMode ? 'text-yellow-200' : 'text-yellow-900'}`}>{event.travelTime.toFixed(2)}s</div>
                                </div>
                                <div className={`rounded p-2 ${darkMode ? 'bg-yellow-900/40' : 'bg-yellow-100'}`}>
                                  <div className={`font-medium ${darkMode ? 'text-yellow-400' : 'text-yellow-700'}`}>Charging Time</div>
                                  <div className={`font-bold ${darkMode ? 'text-yellow-200' : 'text-yellow-900'}`}>{event.chargingTime.toFixed(2)}s</div>
                                </div>
                              </div>
                              
                              {/* Battery Change */}
                              <div className="grid grid-cols-2 gap-2 text-xs">
                                <div className={`rounded p-2 ${darkMode ? 'bg-red-900/30' : 'bg-red-50'}`}>
                                  <div className={`font-medium ${darkMode ? 'text-red-400' : 'text-red-600'}`}>Battery on Arrival</div>
                                  <div className={`font-bold ${darkMode ? 'text-red-200' : 'text-red-900'}`}>{event.batteryBefore.toFixed(1)}%</div>
                                </div>
                                <div className={`rounded p-2 ${darkMode ? 'bg-green-900/30' : 'bg-green-50'}`}>
                                  <div className={`font-medium ${darkMode ? 'text-green-400' : 'text-green-600'}`}>After Charging</div>
                                  <div className={`font-bold ${darkMode ? 'text-green-200' : 'text-green-900'}`}>{event.batteryAfter.toFixed(1)}%</div>
                                </div>
                              </div>
                              
                              {/* Total Time */}
                              <div className={`mt-2 rounded p-2 text-xs ${darkMode ? 'bg-yellow-900/40' : 'bg-yellow-100'}`}>
                                <div className={`font-medium ${darkMode ? 'text-yellow-400' : 'text-yellow-700'}`}>Total Duration</div>
                                <div className={`font-bold ${darkMode ? 'text-yellow-200' : 'text-yellow-900'}`}>{event.duration.toFixed(2)}s (Start: {event.startTime.toFixed(1)}s)</div>
                              </div>
                              
                              {idx < robot.tasks.length - 1 && (
                                <div className={`mt-2 text-center text-xs ${darkMode ? 'text-yellow-500' : 'text-yellow-500'}`}>↓</div>
                              )}
                            </div>
                          );
                        }
                        
                        // Render normal task
                        const task = event as Task;
                        return (
                          <div key={task.id} className={`border rounded-lg p-3 hover:shadow-md transition-shadow ${
                            darkMode ? 'border-slate-600 bg-slate-700' : 'border-slate-200 bg-white'
                          }`}>
                            <div className="flex items-center justify-between mb-2">
                              <span className={`text-sm font-bold px-2 py-1 rounded ${
                                darkMode ? 'text-slate-200 bg-slate-600' : 'text-slate-900 bg-slate-100'
                              }`}>
                                {task.id}
                              </span>
                              <span className={`text-xs ${darkMode ? 'text-slate-400' : 'text-slate-500'}`}>
                                Event {idx + 1} of {robot.tasks.length}
                              </span>
                            </div>
                            
                            {/* Route Information */}
                            <div className={`rounded p-2 mb-2 text-xs space-y-1 ${darkMode ? 'bg-slate-600' : 'bg-slate-50'}`}>
                              <div className="flex items-center justify-between">
                                <span className={darkMode ? 'text-slate-400' : 'text-slate-600'}>From:</span>
                                <span className={`font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>
                                  Node {task.fromNode} ({task.fromCoords?.x.toFixed(1)}, {task.fromCoords?.y.toFixed(1)})
                                </span>
                              </div>
                              <div className="flex items-center justify-between">
                                <span className={darkMode ? 'text-slate-400' : 'text-slate-600'}>To:</span>
                                <span className={`font-medium ${darkMode ? 'text-slate-200' : 'text-slate-900'}`}>
                                  Node {task.toNode} ({task.toCoords?.x.toFixed(1)}, {task.toCoords?.y.toFixed(1)})
                                </span>
                              </div>
                            </div>
                            
                            {/* Timing Information */}
                            <div className="grid grid-cols-2 gap-2 text-xs mb-2">
                              <div className={`rounded p-2 ${darkMode ? 'bg-purple-900/30' : 'bg-purple-50'}`}>
                                <div className={`font-medium ${darkMode ? 'text-purple-400' : 'text-purple-600'}`}>Travel Time</div>
                                <div className={`font-bold ${darkMode ? 'text-purple-200' : 'text-purple-900'}`}>{task.travelTime.toFixed(2)}s</div>
                              </div>
                              <div className={`rounded p-2 ${darkMode ? 'bg-green-900/30' : 'bg-green-50'}`}>
                                <div className={`font-medium ${darkMode ? 'text-green-400' : 'text-green-600'}`}>Execution Time</div>
                                <div className={`font-bold ${darkMode ? 'text-green-200' : 'text-green-900'}`}>{task.executionTime.toFixed(2)}s</div>
                              </div>
                            </div>
                            
                            {/* Timeline & Battery */}
                            <div className="grid grid-cols-2 gap-2 text-xs">
                              <div className={`rounded p-2 ${darkMode ? 'bg-blue-900/30' : 'bg-blue-50'}`}>
                                <div className={`font-medium ${darkMode ? 'text-blue-400' : 'text-blue-600'}`}>Cumulative Time</div>
                                <div className={`font-bold ${darkMode ? 'text-blue-200' : 'text-blue-900'}`}>{task.cumulativeTime.toFixed(2)}s</div>
                              </div>
                              <div className={`rounded p-2 ${
                                darkMode 
                                  ? (task.batteryLevel > 80 ? 'bg-green-900/30' : task.batteryLevel > 50 ? 'bg-yellow-900/30' : 'bg-red-900/30')
                                  : (task.batteryLevel > 80 ? 'bg-green-50' : task.batteryLevel > 50 ? 'bg-yellow-50' : 'bg-red-50')
                              }`}>
                                <div className={`font-medium ${
                                  darkMode
                                    ? (task.batteryLevel > 80 ? 'text-green-400' : task.batteryLevel > 50 ? 'text-yellow-400' : 'text-red-400')
                                    : (task.batteryLevel > 80 ? 'text-green-600' : task.batteryLevel > 50 ? 'text-yellow-600' : 'text-red-600')
                                }`}>Battery After</div>
                                <div className={`font-bold ${
                                  darkMode
                                    ? (task.batteryLevel > 80 ? 'text-green-200' : task.batteryLevel > 50 ? 'text-yellow-200' : 'text-red-200')
                                    : (task.batteryLevel > 80 ? 'text-green-900' : task.batteryLevel > 50 ? 'text-yellow-900' : 'text-red-900')
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

        {/* Timeline Visualizer Modal */}
        {result && showVisualizer && (
          <TimelineVisualizer
            robots={result.robots}
            makespan={result.makespan}
            onClose={() => setShowVisualizer(false)}
            darkMode={darkMode}
          />
        )}
    </div>
  );
}