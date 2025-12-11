import { useState, useEffect, useRef } from 'react';

interface Robot {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  state: string;
  goal: string; // "Node 1234" format from backend
  itinerary: number; // number of waypoints remaining
  batteryLevel: number;
}

interface Stats {
  totalTasks: number;
  completedTasks: number;
  activeTasks: number;
  totalRobots: number;
  busyRobots: number;
  idleRobots: number;
  avgBattery: number;
  uptime: number;
  efficiency: number; // tasks/minute
}

interface SimulatorState {
  isRunning: boolean;
  isConnected: boolean;
  output: string[];
  robots: Robot[];
  stats: Stats;
  startTime: number | null;
}

type Page = 'dashboard' | 'robots';

function App() {
  const darkMode = true;
  const [currentPage, setCurrentPage] = useState<Page>('dashboard');
  const [numRobots, setNumRobots] = useState(4);
  const [command, setCommand] = useState('');
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [state, setState] = useState<SimulatorState>({
    isRunning: false,
    isConnected: false,
    output: [],
    robots: [],
    stats: {
      totalTasks: 0,
      completedTasks: 0,
      activeTasks: 0,
      totalRobots: numRobots,
      busyRobots: 0,
      idleRobots: numRobots,
      avgBattery: 100,
      uptime: 0,
      efficiency: 0
    },
    startTime: null
  });
  
  const wsRef = useRef<WebSocket | null>(null);
  const outputEndRef = useRef<HTMLDivElement>(null);
  const pollIntervalRef = useRef<NodeJS.Timeout | null>(null);

  // Update stats whenever robot data changes
  useEffect(() => {
    if (state.robots.length > 0 || state.isRunning) {
      const busyRobots = state.robots.filter(r => r.state !== 'IDLE').length;
      const avgBattery = state.robots.length > 0 
        ? state.robots.reduce((sum, r) => sum + r.batteryLevel, 0) / state.robots.length 
        : 100;
      const activeTasks = state.robots.filter(r => r.itinerary && r.itinerary > 0).length;
      
      let uptime = 0;
      let efficiency = 0;
      if (state.startTime) {
        uptime = (Date.now() - state.startTime) / 1000; // seconds
        const minutes = uptime / 60;
        efficiency = minutes > 0 ? state.stats.completedTasks / minutes : 0;
      }

      setState(prev => ({
        ...prev,
        stats: {
          ...prev.stats,
          totalRobots: state.robots.length > 0 ? state.robots.length : prev.stats.totalRobots,
          busyRobots,
          idleRobots: (state.robots.length > 0 ? state.robots.length : prev.stats.totalRobots) - busyRobots,
          avgBattery: Math.round(avgBattery),
          activeTasks,
          uptime,
          efficiency
        }
      }));
    }
  }, [state.robots, state.startTime, state.isRunning]);

  useEffect(() => {
    outputEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [state.output]);

  // Poll robot data when running
  useEffect(() => {
    if (state.isRunning) {
      const pollRobots = async () => {
        try {
          const response = await fetch('http://localhost:3001/api/fleet/robots');
          const data = await response.json();
          setState(prev => ({ ...prev, robots: data.robots }));
        } catch (err) {
          console.error('Failed to fetch robots:', err);
        }
      };

      pollRobots();
      pollIntervalRef.current = setInterval(pollRobots, 100); // 10Hz update rate

      return () => {
        if (pollIntervalRef.current) {
          clearInterval(pollIntervalRef.current);
        }
      };
    } else {
      setState(prev => ({ ...prev, robots: [] }));
    }
  }, [state.isRunning]);

  useEffect(() => {
    const ws = new WebSocket('ws://localhost:3001');
    wsRef.current = ws;

    ws.onopen = () => {
      setState(prev => ({ ...prev, isConnected: true }));
    };

    ws.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data);
        
        switch (message.type) {
          case 'simulator_started':
            setState(prev => ({ 
              ...prev, 
              isRunning: true,
              startTime: Date.now(),
              output: [...prev.output, message.message]
            }));
            break;
          case 'simulator_output':
            // Parse output for task completion
            const output = message.data;
            let newCompletedTasks = prev.stats.completedTasks;
            let newTotalTasks = prev.stats.totalTasks;
            
            // Check for task injection
            const injectMatch = output.match(/injected (\d+) tasks/i);
            if (injectMatch) {
              newTotalTasks += parseInt(injectMatch[1]);
            }
            
            // Check for task completion (example pattern, adjust based on actual output)
            if (output.includes('Task completed') || output.includes('COMPLETED')) {
              newCompletedTasks++;
            }
            
            setState(prev => ({ 
              ...prev, 
              output: [...prev.output, message.data],
              stats: {
                ...prev.stats,
                completedTasks: newCompletedTasks,
                totalTasks: newTotalTasks
              }
            }));
            break;
          case 'simulator_error':
            setState(prev => ({ 
              ...prev, 
              output: [...prev.output, 'ERROR: ' + message.data]
            }));
            break;
          case 'simulator_closed':
            setState(prev => ({ 
              ...prev, 
              isRunning: false,
              output: [...prev.output, 'Simulator exited with code ' + message.code]
            }));
            break;
          case 'simulator_stopped':
            setState(prev => ({ 
              ...prev, 
              isRunning: false,
              output: [...prev.output, message.message]
            }));
            break;
          case 'error':
            setState(prev => ({ 
              ...prev, 
              output: [...prev.output, message.message]
            }));
            break;
        }
      } catch (error) {
        console.error('Failed to parse WebSocket message:', error);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      setState(prev => ({ 
        ...prev, 
        output: [...prev.output, 'WebSocket connection error']
      }));
    };

    ws.onclose = () => {
      setState(prev => ({ 
        ...prev, 
        isConnected: false,
        isRunning: false
      }));
    };

    return () => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.close();
      }
    };
  }, []);

  const startFleetManager = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      // Start the C++ fleet_manager with --cli mode
      wsRef.current.send(JSON.stringify({
        type: 'start_simulator',
        graphId: 1,
        numRobots
      }));
      setState(prev => ({ 
        ...prev, 
        stats: {
          ...prev.stats,
          totalRobots: numRobots,
          idleRobots: numRobots
        }
      }));
    }
  };

  const stopFleetManager = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'stop_simulator'
      }));
    }
  };

  const sendCommand = (e: React.FormEvent) => {
    e.preventDefault();
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN && command.trim()) {
      wsRef.current.send(JSON.stringify({
        type: 'send_command',
        command: command.trim()
      }));
      setState(prev => ({ 
        ...prev, 
        output: [...prev.output, '> ' + command.trim()]
      }));
      setCommand('');
    }
  };

  const sendQuickCommand = (cmd: string) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'send_command',
        command: cmd
      }));
      setState(prev => ({ 
        ...prev, 
        output: [...prev.output, '> ' + cmd]
      }));
    }
  };

  const clearOutput = () => {
    setState(prev => ({ ...prev, output: [] }));
  };

  const getBatteryColor = (level: number) => {
    if (level >= 80) return 'text-green-400';
    if (level >= 50) return 'text-yellow-400';
    if (level >= 20) return 'text-orange-400';
    return 'text-red-400';
  };

  const getBatteryBgColor = (level: number) => {
    if (level >= 80) return 'bg-green-500';
    if (level >= 50) return 'bg-yellow-500';
    if (level >= 20) return 'bg-orange-500';
    return 'bg-red-500';
  };

  const getStateColor = (state: string) => {
    switch (state) {
      case 'IDLE': return 'text-gray-400';
      case 'MOVING': return 'text-blue-400';
      case 'EXECUTING': return 'text-purple-400';
      case 'WAITING': return 'text-yellow-400';
      default: return 'text-gray-400';
    }
  };

  const getStateBadgeColor = (state: string) => {
    switch (state) {
      case 'IDLE': return 'bg-gray-700 text-gray-300';
      case 'MOVING': return 'bg-blue-600 text-white';
      case 'EXECUTING': return 'bg-purple-600 text-white';
      case 'WAITING': return 'bg-yellow-600 text-white';
      default: return 'bg-gray-700 text-gray-300';
    }
  };

  return (
    <div className={`min-h-screen ${darkMode ? 'bg-gray-900 text-gray-100' : 'bg-gray-50 text-gray-900'}`}>
      {/* Navigation */}
      <nav className={`border-b ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
        <div className="max-w-7xl mx-auto px-6 py-3">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-6">
              <h1 className="text-xl font-bold">🤖 Fleet Manager</h1>
              <div className="flex gap-2">
                <button
                  onClick={() => setCurrentPage('dashboard')}
                  className={`px-4 py-2 rounded-md text-sm font-medium transition-colors ${
                    currentPage === 'dashboard'
                      ? 'bg-blue-600 text-white'
                      : darkMode 
                        ? 'text-gray-300 hover:bg-gray-700' 
                        : 'text-gray-600 hover:bg-gray-100'
                  }`}
                >
                  Dashboard
                </button>
                <button
                  onClick={() => setCurrentPage('robots')}
                  className={`px-4 py-2 rounded-md text-sm font-medium transition-colors ${
                    currentPage === 'robots'
                      ? 'bg-blue-600 text-white'
                      : darkMode 
                        ? 'text-gray-300 hover:bg-gray-700' 
                        : 'text-gray-600 hover:bg-gray-100'
                  }`}
                >
                  Robots ({state.robots.length})
                </button>
              </div>
            </div>
            <div className="flex items-center gap-4">
              <div className="flex items-center gap-2">
                <div className={`w-2 h-2 rounded-full ${state.isConnected ? 'bg-green-500' : 'bg-red-500'}`} />
                <span className="text-xs">{state.isConnected ? 'Connected' : 'Disconnected'}</span>
              </div>
              {!state.isRunning ? (
                <button 
                  onClick={startFleetManager} 
                  disabled={!state.isConnected}
                  className="px-4 py-1.5 text-sm bg-green-600 text-white rounded-md hover:bg-green-700 disabled:opacity-50 font-medium"
                >
                  Start Fleet
                </button>
              ) : (
                <button 
                  onClick={stopFleetManager} 
                  className="px-4 py-1.5 text-sm bg-red-600 text-white rounded-md hover:bg-red-700 font-medium"
                >
                  Stop Fleet
                </button>
              )}
            </div>
          </div>
        </div>
      </nav>

      <div className="max-w-7xl mx-auto p-6 space-y-6">
        {currentPage === 'dashboard' ? (
          // DASHBOARD PAGE
          <>
            {/* Stats Overview */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
              {/* Efficiency Card */}
              <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gradient-to-br from-blue-900/50 to-blue-800/30 border-blue-700/50' : 'bg-gradient-to-br from-blue-50 to-blue-100 border-blue-200'}`}>
                <div className="flex items-start justify-between">
                  <div>
                    <p className="text-sm font-medium text-blue-300">Efficiency</p>
                    <p className="text-3xl font-bold text-white mt-2">
                      {state.stats.efficiency.toFixed(1)}
                    </p>
                    <p className="text-xs text-blue-200 mt-1">tasks/min</p>
                  </div>
                  <div className="text-3xl">📊</div>
                </div>
                <div className="mt-4 pt-4 border-t border-blue-700/50">
                  <p className="text-xs text-blue-200">
                    {state.stats.completedTasks} / {state.stats.totalTasks} tasks completed
                  </p>
                </div>
              </div>

              {/* Robots Card */}
              <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gradient-to-br from-purple-900/50 to-purple-800/30 border-purple-700/50' : 'bg-gradient-to-br from-purple-50 to-purple-100 border-purple-200'}`}>
                <div className="flex items-start justify-between">
                  <div>
                    <p className="text-sm font-medium text-purple-300">Fleet Status</p>
                    <p className="text-3xl font-bold text-white mt-2">
                      {state.stats.busyRobots}/{state.stats.totalRobots}
                    </p>
                    <p className="text-xs text-purple-200 mt-1">robots active</p>
                  </div>
                  <div className="text-3xl">🤖</div>
                </div>
                <div className="mt-4 pt-4 border-t border-purple-700/50">
                  <p className="text-xs text-purple-200">
                    {state.stats.idleRobots} idle · {state.stats.activeTasks} tasks running
                  </p>
                </div>
              </div>

              {/* Battery Card */}
              <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gradient-to-br from-green-900/50 to-green-800/30 border-green-700/50' : 'bg-gradient-to-br from-green-50 to-green-100 border-green-200'}`}>
                <div className="flex items-start justify-between">
                  <div>
                    <p className="text-sm font-medium text-green-300">Avg Battery</p>
                    <p className="text-3xl font-bold text-white mt-2">
                      {state.stats.avgBattery}%
                    </p>
                    <p className="text-xs text-green-200 mt-1">fleet average</p>
                  </div>
                  <div className="text-3xl">🔋</div>
                </div>
                <div className="mt-4 pt-4 border-t border-green-700/50">
                  <div className="w-full bg-gray-700/50 rounded-full h-2">
                    <div 
                      className={`h-2 rounded-full transition-all ${getBatteryBgColor(state.stats.avgBattery)}`}
                      style={{ width: `${state.stats.avgBattery}%` }}
                    />
                  </div>
                </div>
              </div>

              {/* Uptime Card */}
              <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gradient-to-br from-orange-900/50 to-orange-800/30 border-orange-700/50' : 'bg-gradient-to-br from-orange-50 to-orange-100 border-orange-200'}`}>
                <div className="flex items-start justify-between">
                  <div>
                    <p className="text-sm font-medium text-orange-300">Uptime</p>
                    <p className="text-3xl font-bold text-white mt-2">
                      {Math.floor(state.stats.uptime / 60)}m
                    </p>
                    <p className="text-xs text-orange-200 mt-1">{Math.floor(state.stats.uptime % 60)}s elapsed</p>
                  </div>
                  <div className="text-3xl">⏱️</div>
                </div>
                <div className="mt-4 pt-4 border-t border-orange-700/50">
                  <p className="text-xs text-orange-200">
                    {state.isRunning ? 'System operational' : 'System offline'}
                  </p>
                </div>
              </div>
            </div>

            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              {/* Left Column: Controls */}
              <div className="space-y-6">
                {/* Fleet Configuration */}
                {!state.isRunning && (
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                    <h2 className="text-lg font-semibold mb-4 flex items-center gap-2">
                      <span>⚙️</span>
                      Fleet Configuration
                    </h2>
                    <div className="space-y-4">
                      <div>
                        <label className={`block text-sm font-medium mb-2 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>
                          Number of Robots
                        </label>
                        <input 
                          type="number" 
                          min="1" 
                          max="20" 
                          value={numRobots} 
                          onChange={(e) => setNumRobots(Number(e.target.value))}
                          className={`w-full px-4 py-2 text-lg rounded-lg border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'}`}
                        />
                        <p className="text-xs text-gray-500 mt-1">Configure fleet size before starting (1-20 robots)</p>
                      </div>
                      <button 
                        onClick={startFleetManager} 
                        disabled={!state.isConnected}
                        className="w-full px-6 py-3 text-lg bg-green-600 hover:bg-green-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-bold transition-colors shadow-lg">
                        🚀 Start Fleet Manager
                      </button>
                    </div>
                  </div>
                )}

                {/* Quick Actions */}
                <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                  <h2 className="text-lg font-semibold mb-4 flex items-center gap-2">
                    <span>⚡</span>
                    Quick Actions
                  </h2>
                  <div className="grid grid-cols-2 gap-3">
                    <button onClick={() => sendQuickCommand('inject 3')} disabled={!state.isRunning}
                      className="px-4 py-3 text-sm bg-blue-600 hover:bg-blue-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                      Inject 3 Tasks<span className="block text-xs opacity-75">Scenario B</span>
                    </button>
                    <button onClick={() => sendQuickCommand('inject 5')} disabled={!state.isRunning}
                      className="px-4 py-3 text-sm bg-blue-600 hover:bg-blue-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                      Inject 5 Tasks<span className="block text-xs opacity-75">Scenario B</span>
                    </button>
                    <button onClick={() => sendQuickCommand('inject 10')} disabled={!state.isRunning}
                      className="px-4 py-3 text-sm bg-purple-600 hover:bg-purple-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                      Inject 10 Tasks<span className="block text-xs opacity-75">Scenario C</span>
                    </button>
                    <button onClick={() => sendQuickCommand('status')} disabled={!state.isRunning}
                      className="px-4 py-3 text-sm bg-green-600 hover:bg-green-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                      Status Check<span className="block text-xs opacity-75">Robot States</span>
                    </button>
                  </div>
                </div>

                {/* Commands */}
                <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                  <h2 className="text-lg font-semibold mb-4 flex items-center gap-2">
                    <span>🎮</span>
                    System Commands
                  </h2>
                  <div className="space-y-4">
                    <div className="grid grid-cols-2 gap-2">
                      <button onClick={() => sendQuickCommand('status')} disabled={!state.isRunning}
                        className="px-4 py-2 text-sm bg-gray-700 hover:bg-gray-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                        📊 Status
                      </button>
                      <button onClick={() => sendQuickCommand('stats')} disabled={!state.isRunning}
                        className="px-4 py-2 text-sm bg-gray-700 hover:bg-gray-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                        📈 Stats
                      </button>
                      <button onClick={() => sendQuickCommand('help')} disabled={!state.isRunning}
                        className="px-4 py-2 text-sm bg-gray-700 hover:bg-gray-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                        ❓ Help
                      </button>
                      <button onClick={clearOutput}
                        className="px-4 py-2 text-sm bg-gray-700 hover:bg-gray-600 text-white rounded-lg font-medium transition-colors">
                        🗑️ Clear
                      </button>
                    </div>
                    
                    <div className="pt-3 border-t border-gray-700">
                      <label className="block text-sm font-medium mb-2 text-gray-300">
                        💬 Custom Command
                      </label>
                      <form onSubmit={sendCommand} className="flex gap-2">
                        <input
                          type="text"
                          value={command}
                          onChange={(e) => setCommand(e.target.value)}
                          placeholder="Type command (e.g., inject 20, status)..."
                          disabled={!state.isRunning}
                          className={`flex-1 px-4 py-2 text-sm rounded-lg border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100 placeholder-gray-500' : 'bg-white border-gray-300 text-gray-900'} disabled:opacity-50`}
                        />
                        <button
                          type="submit"
                          disabled={!state.isRunning || !command.trim()}
                          className="px-6 py-2 text-sm bg-blue-600 hover:bg-blue-700 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                          Send
                        </button>
                      </form>
                      <p className="text-xs text-gray-500 mt-2">
                        Available: inject &lt;N&gt;, status, stats, help, quit
                      </p>
                    </div>
                  </div>
                </div>
              </div>

              {/* Right Column: Output Terminal */}
              <div className={`rounded-xl border ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                <div className="p-6 border-b border-gray-700">
                  <h2 className="text-lg font-semibold flex items-center gap-2">
                    <span>📟</span>
                    System Output
                  </h2>
                </div>
                <div className={`p-4 h-[600px] overflow-y-auto font-mono text-xs ${darkMode ? 'bg-gray-900' : 'bg-gray-50'}`}>
                  {state.output.length === 0 ? (
                    <div className="text-gray-500 text-center py-8">
                      No output yet. Start the fleet to see system messages.
                    </div>
                  ) : (
                    state.output.map((line, i) => (
                      <div key={i} className={`mb-1 ${line.startsWith('>') ? 'text-blue-400 font-bold' : line.includes('ERROR') ? 'text-red-400' : 'text-gray-300'}`}>
                        {line}
                      </div>
                    ))
                  )}
                  <div ref={outputEndRef} />
                </div>
              </div>
            </div>
          </>
        ) : (
          // ROBOTS PAGE
          <div>
            <div className="mb-6">
              <h2 className="text-2xl font-bold">Robot Fleet Monitor</h2>
              <p className={`text-sm ${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
                Real-time status, battery levels, and position tracking
              </p>
            </div>

            {!state.isRunning ? (
              <div className={`rounded-xl border p-12 text-center ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                <div className="text-6xl mb-4">🤖</div>
                <h3 className="text-xl font-semibold mb-2">Fleet Not Running</h3>
                <p className={`${darkMode ? 'text-gray-400' : 'text-gray-600'} mb-4`}>
                  Start the fleet manager from the Dashboard to see live robot data
                </p>
              </div>
            ) : state.robots.length === 0 ? (
              <div className={`rounded-xl border p-12 text-center ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                <div className="text-6xl mb-4 animate-pulse">⏳</div>
                <h3 className="text-xl font-semibold mb-2">Loading Robots...</h3>
                <p className={`${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
                  Fetching robot data from backend...
                </p>
              </div>
            ) : (
              <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
                {state.robots.map(robot => (
                  <div key={robot.id} className={`rounded-xl border p-5 hover:shadow-lg transition-shadow ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                    <div className="flex items-center justify-between mb-4">
                      <h3 className="text-lg font-bold">Robot #{robot.id}</h3>
                      <span className={`px-2.5 py-1 rounded-lg text-xs font-bold ${getStateBadgeColor(robot.state)}`}>
                        {robot.state}
                      </span>
                    </div>

                    {/* Battery */}
                    <div className="mb-3">
                      <div className="flex items-center justify-between mb-1">
                        <span className="text-xs text-gray-400">🔋 Battery</span>
                        <span className={`text-sm font-bold ${
                          robot.batteryLevel >= 80 ? 'text-green-400' :
                          robot.batteryLevel >= 50 ? 'text-yellow-400' :
                          robot.batteryLevel >= 20 ? 'text-orange-400' :
                          'text-red-400'
                        }`}>
                          {robot.batteryLevel}%
                        </span>
                      </div>
                      <div className={`h-2 rounded-full ${darkMode ? 'bg-gray-700' : 'bg-gray-200'} overflow-hidden`}>
                        <div 
                          className={`h-full transition-all duration-300 ${
                            robot.batteryLevel >= 80 ? 'bg-green-500' :
                            robot.batteryLevel >= 50 ? 'bg-yellow-500' :
                            robot.batteryLevel >= 20 ? 'bg-orange-500' :
                            'bg-red-500'
                          }`}
                          style={{ width: `${robot.batteryLevel}%` }}
                        />
                      </div>
                    </div>

                    {/* Position */}
                    <div className={`p-2 rounded text-xs mb-2 ${darkMode ? 'bg-gray-700' : 'bg-gray-100'}`}>
                      <div className="flex justify-between mb-1">
                        <span className="text-gray-400">📍 Position:</span>
                        <span className="font-mono">({robot.x}, {robot.y})</span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-gray-400">⚡ Velocity:</span>
                        <span className="font-mono">({robot.vx.toFixed(1)}, {robot.vy.toFixed(1)})</span>
                      </div>
                    </div>

                    {/* Goal */}
                    {robot.goal && (
                      <div className={`p-2 rounded text-xs mb-2 ${darkMode ? 'bg-blue-900/30 border border-blue-700' : 'bg-blue-100 border border-blue-300'}`}>
                        <div className="text-blue-400 mb-1">🎯 Current Goal</div>
                        <div className="font-mono">{robot.goal}</div>
                      </div>
                    )}

                    {/* Itinerary */}
                    {robot.itinerary > 0 && (
                      <div className={`p-2 rounded text-xs ${darkMode ? 'bg-purple-900/30 border border-purple-700' : 'bg-purple-100 border border-purple-300'}`}>
                        <div className="text-purple-400 mb-1">📋 Task Queue</div>
                        <div className="font-mono">
                          {robot.itinerary} waypoint{robot.itinerary > 1 ? 's' : ''} remaining
                        </div>
                      </div>
                    )}

                    {/* Idle indicator */}
                    {robot.state === 'IDLE' && robot.itinerary === 0 && !robot.goal && (
                      <div className={`p-2 rounded text-xs text-center ${darkMode ? 'bg-gray-700' : 'bg-gray-100'} text-gray-400`}>
                        💤 Awaiting tasks
                      </div>
                    )}
                  </div>
                ))}
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}

export default App;