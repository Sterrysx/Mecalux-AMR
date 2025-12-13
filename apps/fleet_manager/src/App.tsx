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

type Page = 'control' | 'dashboard' | 'robots';

interface TaskHistory {
  timestamp: number;
  completed: number;
}

function App() {
  const darkMode = true;
  const [currentPage, setCurrentPage] = useState<Page>('dashboard');
  const [numRobots, setNumRobots] = useState(4);
  const [command, setCommand] = useState('');
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [taskHistory, setTaskHistory] = useState<TaskHistory[]>([]);
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
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

  // Helper function to categorize and style messages
  const getMessageStyle = (line: string) => {
    if (line.startsWith('>')) {
      return { icon: '▶', color: 'text-blue-400', bg: darkMode ? 'bg-blue-900/20' : 'bg-blue-100', border: 'border-l-4 border-blue-500' };
    }
    if (line.includes('ERROR') || line.includes('Error')) {
      return { icon: '❌', color: 'text-red-400', bg: darkMode ? 'bg-red-900/20' : 'bg-red-100', border: 'border-l-4 border-red-500' };
    }
    if (line.includes('[MainLoop]')) {
      return { icon: '🔄', color: 'text-purple-400', bg: darkMode ? 'bg-purple-900/10' : 'bg-purple-50', border: 'border-l-4 border-purple-500' };
    }
    if (line.includes('[Insertion]')) {
      return { icon: '📌', color: 'text-yellow-400', bg: darkMode ? 'bg-yellow-900/10' : 'bg-yellow-50', border: 'border-l-4 border-yellow-500' };
    }
    if (line.includes('[FleetManager]')) {
      return { icon: '🚀', color: 'text-green-400', bg: darkMode ? 'bg-green-900/10' : 'bg-green-50', border: 'border-l-4 border-green-500' };
    }
    if (line.includes('[Bridge]')) {
      return { icon: '🌉', color: 'text-cyan-400', bg: darkMode ? 'bg-cyan-900/10' : 'bg-cyan-50', border: 'border-l-4 border-cyan-500' };
    }
    if (line.includes('[RobotDriver]')) {
      return { icon: '🤖', color: 'text-indigo-400', bg: darkMode ? 'bg-indigo-900/10' : 'bg-indigo-50', border: 'border-l-4 border-indigo-500' };
    }
    if (line.includes('SCENARIO')) {
      return { icon: '📋', color: 'text-orange-400', bg: darkMode ? 'bg-orange-900/20' : 'bg-orange-100', border: 'border-l-4 border-orange-500' };
    }
    if (line.includes('╔') || line.includes('║') || line.includes('╚')) {
      return { icon: '📊', color: 'text-gray-300', bg: darkMode ? 'bg-gray-800' : 'bg-gray-100', border: 'border-l-4 border-gray-600' };
    }
    if (line.includes('amr>')) {
      return { icon: '💻', color: 'text-blue-300', bg: darkMode ? 'bg-blue-900/10' : 'bg-blue-50', border: 'border-l-4 border-blue-400' };
    }
    return { icon: '•', color: 'text-gray-400', bg: 'transparent', border: 'border-l-4 border-transparent' };
  };

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
          setState(prev => ({
            ...prev,
            robots: data.robots.map((newRobot: Robot) => {
              // Preserve battery level from previous state if manually set
              const existingRobot = prev.robots.find(r => r.id === newRobot.id);
              return existingRobot 
                ? { ...newRobot, batteryLevel: existingRobot.batteryLevel }
                : newRobot;
            })
          }));
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
            console.log('[DEBUG] Received output:', message.data?.substring(0, 100)); // Log first 100 chars
            setState(prev => {
              const output = message.data;
              let newCompletedTasks = prev.stats.completedTasks;
              let newTotalTasks = prev.stats.totalTasks;
              
              // Prevent duplicate messages
              const lastOutput = prev.output[prev.output.length - 1];
              if (lastOutput === output) {
                console.log('[DEBUG] Skipping duplicate message');
                return prev; // Skip duplicate
              }
              
              // Check for task injection
              const injectMatch = output.match(/injected (\d+) tasks/i);
              if (injectMatch) {
                console.log('[DEBUG] Task injection found:', injectMatch[1]);
                newTotalTasks += parseInt(injectMatch[1]);
              }
              
              // Parse tasks completed from CLI output: "Tasks completed: 3 / 100"
              const tasksMatch = output.match(/Tasks completed:\s*(\d+)\s*\/\s*(\d+)/);
              if (tasksMatch) {
                console.log('[DEBUG] Tasks match found:', tasksMatch[1], '/', tasksMatch[2]);
                newCompletedTasks = parseInt(tasksMatch[1]);
                newTotalTasks = parseInt(tasksMatch[2]);
              } else if (output.includes('Tasks completed')) {
                console.log('[DEBUG] "Tasks completed" found but regex did not match. Full output:', output);
              }
              
              // Check for task completion events
              if (output.includes('Task completed') || output.includes('COMPLETED')) {
                newCompletedTasks++;
                console.log('[DEBUG] Task completion detected, incrementing to:', newCompletedTasks);
              }
              
              // Log if stats changed
              if (newCompletedTasks !== prev.stats.completedTasks || newTotalTasks !== prev.stats.totalTasks) {
                console.log('[DEBUG] Updating stats from', prev.stats.completedTasks, '/', prev.stats.totalTasks, 'to', newCompletedTasks, '/', newTotalTasks);
              }
              
              return {
                ...prev, 
                output: [...prev.output, message.data],
                stats: {
                  ...prev.stats,
                  completedTasks: newCompletedTasks,
                  totalTasks: newTotalTasks
                }
              };
            });
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

  const updateRobotBattery = (robotId: number, newLevel: number) => {
    setState(prev => ({
      ...prev,
      robots: prev.robots.map(r => 
        r.id === robotId ? { ...r, batteryLevel: newLevel } : r
      )
    }));
  };

  // Track task completion history for charts
  useEffect(() => {
    if (state.isRunning) {
      const interval = setInterval(() => {
        setTaskHistory(prev => {
          const newEntry = {
            timestamp: Date.now(),
            completed: state.stats.completedTasks
          };
          // Keep last 30 data points
          const updated = [...prev, newEntry].slice(-30);
          return updated;
        });
      }, 2000); // Update every 2 seconds

      return () => clearInterval(interval);
    }
  }, [state.isRunning, state.stats.completedTasks]);

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
    <div className={`min-h-screen flex ${darkMode ? 'bg-gray-900 text-gray-100' : 'bg-gray-50 text-gray-900'}`}>
      {/* Sidebar Navigation */}
      <aside className={`${sidebarCollapsed ? 'w-20' : 'w-64'} bg-gradient-to-b from-blue-900 to-blue-800 text-white flex-shrink-0 transition-all duration-300 shadow-xl`}>
        <div className="p-6">
          {/* User Profile */}
          <div className={`${sidebarCollapsed ? 'flex justify-center' : ''} mb-8`}>
            <div className="bg-white/10 backdrop-blur-sm rounded-2xl p-4 text-center">
              <div className="w-16 h-16 bg-white rounded-full mx-auto mb-3 flex items-center justify-center">
                <span className="text-3xl">👤</span>
              </div>
              {!sidebarCollapsed && (
                <>
                  <h3 className="font-bold text-lg">FLEET ADMIN</h3>
                  <p className="text-xs text-blue-200">admin@mecalux.com</p>
                </>
              )}
            </div>
          </div>

          {/* Navigation Menu */}
          <nav className="space-y-2">
            <button
              onClick={() => setCurrentPage('dashboard')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${
                currentPage === 'dashboard' 
                  ? 'bg-white/20 shadow-lg' 
                  : 'hover:bg-white/10'
              }`}
            >
              <span className="text-xl">🏠</span>
              {!sidebarCollapsed && <span className="font-medium">Dashboard</span>}
            </button>
            
            <button
              onClick={() => setCurrentPage('robots')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${
                currentPage === 'robots' 
                  ? 'bg-white/20 shadow-lg' 
                  : 'hover:bg-white/10'
              }`}
            >
              <span className="text-xl">🤖</span>
              {!sidebarCollapsed && <span className="font-medium">Fleet</span>}
            </button>

            <button
              onClick={() => setCurrentPage('control')}
              className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-all ${
                currentPage === 'control' 
                  ? 'bg-white/20 shadow-lg' 
                  : 'hover:bg-white/10'
              }`}
            >
              <span className="text-xl">🎮</span>
              {!sidebarCollapsed && <span className="font-medium">Control</span>}
            </button>

            <button className="w-full flex items-center gap-3 px-4 py-3 rounded-lg hover:bg-white/10 transition-all">
              <span className="text-xl">📊</span>
              {!sidebarCollapsed && <span className="font-medium">Analytics</span>}
            </button>

            <button className="w-full flex items-center gap-3 px-4 py-3 rounded-lg hover:bg-white/10 transition-all">
              <span className="text-xl">⚙️</span>
              {!sidebarCollapsed && <span className="font-medium">Settings</span>}
            </button>
          </nav>
        </div>

        {/* Sidebar Toggle */}
        <button 
          onClick={() => setSidebarCollapsed(!sidebarCollapsed)}
          className="absolute bottom-6 right-4 bg-white/20 hover:bg-white/30 p-2 rounded-lg transition-all"
        >
          <span className="text-xl">{sidebarCollapsed ? '→' : '←'}</span>
        </button>
      </aside>

      {/* Main Content */}
      <div className="flex-1 overflow-auto">
        {/* Top Bar */}
        <header className={`${darkMode ? 'bg-gray-800 border-b border-gray-700' : 'bg-white'} shadow-sm px-8 py-4 flex items-center justify-between sticky top-0 z-10`}>
          <div>
            <h1 className={`text-2xl font-bold ${darkMode ? 'text-gray-100' : 'text-gray-800'}`}>Dashboard</h1>
            <p className={`text-sm ${darkMode ? 'text-gray-400' : 'text-gray-500'}`}>Real-time fleet monitoring system</p>
          </div>
          <div className="flex items-center gap-4">
            <div className={`flex items-center gap-2 px-3 py-2 rounded-lg ${darkMode ? 'bg-gray-700' : 'bg-gray-100'}`}>
              <div className={`w-2 h-2 rounded-full ${state.isConnected ? 'bg-green-500 animate-pulse' : 'bg-red-500'}`} />
              <span className={`text-sm font-medium ${darkMode ? 'text-gray-200' : 'text-gray-900'}`}>{state.isConnected ? 'Connected' : 'Disconnected'}</span>
            </div>
            {!state.isRunning ? (
              <button 
                onClick={startFleetManager} 
                disabled={!state.isConnected}
                className="px-6 py-2 bg-green-500 text-white rounded-lg hover:bg-green-600 disabled:opacity-50 font-medium shadow-lg hover:shadow-xl transition-all flex items-center gap-2"
              >
                <span>▶</span>
                Start Fleet
              </button>
            ) : (
              <button 
                onClick={stopFleetManager} 
                className="px-6 py-2 bg-red-500 text-white rounded-lg hover:bg-red-600 font-medium shadow-lg hover:shadow-xl transition-all flex items-center gap-2"
              >
                <span>⏹</span>
                Stop Fleet
              </button>
            )}
          </div>
        </header>

        <main className={`p-8 ${darkMode ? 'bg-gray-900' : 'bg-gray-100'}`}>
          {currentPage === 'dashboard' ? (
          // LIVE DASHBOARD PAGE
          <>
            {!state.isRunning ? (
              <div className={`rounded-2xl shadow-xl p-16 text-center ${darkMode ? 'bg-gray-800 border border-gray-700' : 'bg-white'}`}>
                <div className="text-8xl mb-6 animate-pulse">📊</div>
                <h3 className={`text-3xl font-bold mb-3 ${darkMode ? 'text-gray-100' : 'text-gray-800'}`}>Dashboard Ready</h3>
                <p className={`text-lg mb-6 ${darkMode ? 'text-gray-400' : 'text-gray-500'}`}>
                  Start the fleet to see live metrics and robot performance data
                </p>
                <button 
                  onClick={startFleetManager} 
                  disabled={!state.isConnected}
                  className="px-8 py-3 text-lg bg-gradient-to-r from-green-500 to-green-600 text-white rounded-xl hover:from-green-600 hover:to-green-700 disabled:opacity-50 font-medium inline-flex items-center gap-2 shadow-lg"
                >
                  <span>▶</span>
                  Start Fleet Manager
                </button>
              </div>
            ) : (
              <>
                {/* Top Stats Cards - Professional Style */}
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-6">
                  {/* Tasks Completed */}
                  <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                    <div className="flex items-center justify-between mb-4">
                      <div className="bg-gradient-to-br from-blue-500 to-blue-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                        📊
                      </div>
                      <div className="bg-blue-500/20 text-blue-400 px-3 py-1 rounded-full text-sm font-bold">
                        +{state.stats.efficiency.toFixed(1)}/min
                      </div>
                    </div>
                    <p className={`text-sm mb-1 ${darkMode ? 'text-gray-400' : 'text-gray-500'}`}>Tasks Completed</p>
                    <p className={`text-3xl font-bold ${darkMode ? 'text-gray-100' : 'text-gray-800'}`}>{state.stats.completedTasks}</p>
                    <p className={`text-xs mt-2 ${darkMode ? 'text-gray-500' : 'text-gray-400'}`}>of {state.stats.totalTasks} total tasks</p>
                  </div>

                  {/* Active Robots */}
                  <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                    <div className="flex items-center justify-between mb-4">
                      <div className="bg-gradient-to-br from-orange-500 to-orange-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                        🤖
                      </div>
                      <div className="bg-orange-500/20 text-orange-400 px-3 py-1 rounded-full text-sm font-bold">
                        {state.stats.busyRobots} Active
                      </div>
                    </div>
                    <p className="text-sm text-gray-400 mb-1">Fleet Status</p>
                    <p className="text-3xl font-bold text-gray-100">{state.stats.totalRobots}</p>
                    <p className="text-xs text-gray-500 mt-2">{state.stats.idleRobots} idle robots</p>
                  </div>

                  {/* Battery Health */}
                  <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                    <div className="flex items-center justify-between mb-4">
                      <div className="bg-gradient-to-br from-green-500 to-green-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                        🔋
                      </div>
                      <div className={`${state.stats.avgBattery >= 80 ? 'bg-green-500/20 text-green-400' : state.stats.avgBattery >= 50 ? 'bg-yellow-500/20 text-yellow-400' : 'bg-red-500/20 text-red-400'} px-3 py-1 rounded-full text-sm font-bold`}>
                        {state.stats.avgBattery >= 80 ? '●●●' : state.stats.avgBattery >= 50 ? '●●○' : '●○○'}
                      </div>
                    </div>
                    <p className="text-sm text-gray-400 mb-1">Avg Battery</p>
                    <p className="text-3xl font-bold text-gray-100">{state.stats.avgBattery}%</p>
                    <div className="w-full bg-gray-700 rounded-full h-2 mt-3">
                      <div 
                        className={`h-2 rounded-full transition-all duration-500 ${getBatteryBgColor(state.stats.avgBattery)}`}
                        style={{ width: `${state.stats.avgBattery}%` }}
                      />
                    </div>
                  </div>

                  {/* Efficiency Rating */}
                  <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                    <div className="flex items-center justify-between mb-4">
                      <div className="bg-gradient-to-br from-yellow-500 to-yellow-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                        ⭐
                      </div>
                      <div className="bg-yellow-500/20 text-yellow-400 px-3 py-1 rounded-full text-sm font-bold">
                        Rating
                      </div>
                    </div>
                    <p className="text-sm text-gray-400 mb-1">Efficiency Score</p>
                    <p className="text-3xl font-bold text-gray-100">{state.stats.efficiency > 0 ? (Math.min(10, state.stats.efficiency) / 10 * 10).toFixed(1) : '0.0'}</p>
                    <p className="text-xs text-gray-500 mt-2">Uptime: {Math.floor(state.stats.uptime / 60)}m {Math.floor(state.stats.uptime % 60)}s</p>
                  </div>
                </div>

                {/* Main Dashboard Grid */}
                <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-6">
                  {/* Task Completion Chart */}
                  <div className="lg:col-span-2 bg-white rounded-2xl shadow-lg p-6">
                    <div className="flex items-center justify-between mb-6">
                      <h3 className="text-lg font-bold text-gray-100">Task Completion Over Time</h3>
                      <div className="flex gap-2">
                        <div className="flex items-center gap-2 text-xs">
                          <div className="w-3 h-3 bg-blue-500 rounded"></div>
                          <span className="text-gray-600">Completed</span>
                        </div>
                        <div className="flex items-center gap-2 text-xs">
                          <div className="w-3 h-3 bg-orange-400 rounded"></div>
                          <span className="text-gray-600">Active</span>
                        </div>
                      </div>
                    </div>
                    
                    {/* Simple Bar Chart */}
                    <div className="h-64 flex items-end justify-between gap-2">
                      {taskHistory.length > 0 ? (
                        taskHistory.slice(-15).map((point, i) => {
                          const maxValue = Math.max(...taskHistory.map(p => p.completed), 1);
                          const height = (point.completed / maxValue) * 100;
                          return (
                            <div key={i} className="flex-1 flex flex-col items-center">
                              <div className="w-full bg-gradient-to-t from-blue-500 to-blue-400 rounded-t-lg transition-all duration-500 hover:from-blue-600 hover:to-blue-500" 
                                   style={{ height: `${height}%`, minHeight: '8px' }}>
                              </div>
                            </div>
                          );
                        })
                      ) : (
                        <div className="w-full h-full flex items-center justify-center text-gray-400">
                          <div className="text-center">
                            <div className="text-4xl mb-2">📈</div>
                            <p>Collecting data...</p>
                          </div>
                        </div>
                      )}
                    </div>
                    
                    <div className="mt-4 pt-4 border-t border-gray-200">
                      <div className="flex justify-between text-sm text-gray-600">
                        <span>Real-time Performance</span>
                        <span className="font-semibold">{state.stats.completedTasks} tasks • {state.stats.efficiency.toFixed(1)}/min</span>
                      </div>
                    </div>
                  </div>

                  {/* Efficiency Meter */}
                  <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
                    <h3 className="text-lg font-bold text-gray-100 mb-6">System Efficiency</h3>
                    <div className="flex items-center justify-center mb-6">
                      <div className="relative w-48 h-48">
                        {/* Circular Progress */}
                        <svg className="w-full h-full transform -rotate-90">
                          <circle
                            cx="96"
                            cy="96"
                            r="85"
                            stroke="#e5e7eb"
                            strokeWidth="12"
                            fill="none"
                          />
                          <circle
                            cx="96"
                            cy="96"
                            r="85"
                            stroke="url(#gradient)"
                            strokeWidth="12"
                            fill="none"
                            strokeDasharray={`${2 * Math.PI * 85}`}
                            strokeDashoffset={`${2 * Math.PI * 85 * (1 - Math.min(state.stats.avgBattery / 100, 1))}`}
                            strokeLinecap="round"
                            className="transition-all duration-1000"
                          />
                          <defs>
                            <linearGradient id="gradient" x1="0%" y1="0%" x2="100%" y2="100%">
                              <stop offset="0%" stopColor="#3b82f6" />
                              <stop offset="100%" stopColor="#8b5cf6" />
                            </linearGradient>
                          </defs>
                        </svg>
                        <div className="absolute inset-0 flex items-center justify-center flex-col">
                          <span className="text-4xl font-bold text-gray-100">{state.stats.avgBattery}%</span>
                          <span className="text-sm text-gray-500">Battery</span>
                        </div>
                      </div>
                    </div>
                    <div className="space-y-3">
                      <div className="flex justify-between items-center">
                        <span className="text-sm text-gray-600">Active Robots</span>
                        <span className="font-semibold text-gray-100">{state.stats.busyRobots}/{state.stats.totalRobots}</span>
                      </div>
                      <div className="flex justify-between items-center">
                        <span className="text-sm text-gray-600">Tasks/Min</span>
                        <span className="font-semibold text-gray-100">{state.stats.efficiency.toFixed(1)}</span>
                      </div>
                      <div className="flex justify-between items-center">
                        <span className="text-sm text-gray-600">Uptime</span>
                        <span className="font-semibold text-gray-100">{Math.floor(state.stats.uptime / 60)}m {Math.floor(state.stats.uptime % 60)}s</span>
                      </div>
                    </div>
                  </div>
                </div>

                {/* Robot Battery Controls */}
                <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
                  <div className="flex items-center justify-between mb-6">
                    <h3 className="text-lg font-bold text-gray-100 flex items-center gap-2">
                      <span>🔋</span>
                      Robot Battery Controls
                    </h3>
                    <span className="text-sm text-gray-500">Adjust individual robot battery levels</span>
                  </div>
                  
                  {state.robots.length === 0 ? (
                    <div className="text-center py-12 text-gray-400">
                      <div className="text-5xl mb-3">⏳</div>
                      <p>Loading robot data...</p>
                    </div>
                  ) : (
                    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                      {state.robots.map(robot => (
                        <div key={robot.id} className="bg-gradient-to-br from-gray-50 to-gray-100 rounded-xl p-4 border border-gray-200 hover:shadow-lg transition-all">
                          <div className="flex items-center justify-between mb-3">
                            <div className="flex items-center gap-2">
                              <div className={`w-10 h-10 rounded-lg flex items-center justify-center text-lg ${
                                robot.state === 'BUSY' ? 'bg-blue-500 text-white' : 
                                robot.state === 'IDLE' ? 'bg-gray-400 text-white' : 
                                'bg-yellow-500 text-white'
                              }`}>
                                🤖
                              </div>
                              <div>
                                <p className="font-bold text-gray-100">Robot #{robot.id}</p>
                                <p className="text-xs text-gray-500">{robot.state}</p>
                              </div>
                            </div>
                          </div>
                          
                          {/* Battery Display */}
                          <div className="mb-3">
                            <div className="flex items-center justify-between mb-2">
                              <span className="text-xs font-medium text-gray-600">Battery Level</span>
                              <span className={`text-lg font-bold ${getBatteryColor(robot.batteryLevel)}`}>
                                {robot.batteryLevel}%
                              </span>
                            </div>
                            <div className="w-full bg-gray-700 rounded-full h-4 overflow-hidden shadow-inner">
                              <div 
                                className={`h-full transition-all duration-300 ${getBatteryBgColor(robot.batteryLevel)} flex items-center justify-end pr-1`}
                                style={{ width: `${robot.batteryLevel}%` }}
                              >
                                {robot.batteryLevel > 20 && (
                                  <span className="text-xs text-white font-bold">⚡</span>
                                )}
                              </div>
                            </div>
                          </div>

                          {/* Battery Control Slider */}
                          <div className="mb-3">
                            <input 
                              type="range" 
                              min="0" 
                              max="100" 
                              value={robot.batteryLevel}
                              onChange={(e) => updateRobotBattery(robot.id, Number(e.target.value))}
                              className="w-full h-2 bg-gray-300 rounded-lg appearance-none cursor-pointer accent-blue-500"
                              style={{
                                background: `linear-gradient(to right, ${
                                  robot.batteryLevel >= 80 ? '#10b981' :
                                  robot.batteryLevel >= 50 ? '#eab308' :
                                  robot.batteryLevel >= 20 ? '#f97316' :
                                  '#ef4444'
                                } ${robot.batteryLevel}%, #d1d5db ${robot.batteryLevel}%)`
                              }}
                            />
                          </div>

                          {/* Quick Battery Actions */}
                          <div className="flex gap-1">
                            <button
                              onClick={() => updateRobotBattery(robot.id, 100)}
                              className="flex-1 px-2 py-1 bg-green-500 hover:bg-green-600 text-white text-xs rounded-lg font-medium transition-colors"
                            >
                              Full
                            </button>
                            <button
                              onClick={() => updateRobotBattery(robot.id, 50)}
                              className="flex-1 px-2 py-1 bg-yellow-500 hover:bg-yellow-600 text-white text-xs rounded-lg font-medium transition-colors"
                            >
                              50%
                            </button>
                            <button
                              onClick={() => updateRobotBattery(robot.id, 20)}
                              className="flex-1 px-2 py-1 bg-orange-500 hover:bg-orange-600 text-white text-xs rounded-lg font-medium transition-colors"
                            >
                              Low
                            </button>
                          </div>

                          {/* Additional Info */}
                          <div className="mt-3 pt-3 border-t border-gray-300 text-xs text-gray-600">
                            <div className="flex justify-between mb-1">
                              <span>Position:</span>
                              <span className="font-mono">({robot.x}, {robot.y})</span>
                            </div>
                            {robot.itinerary > 0 && (
                              <div className="flex justify-between">
                                <span>Tasks:</span>
                                <span className="font-semibold text-blue-600">{robot.itinerary} queued</span>
                              </div>
                            )}
                          </div>
                        </div>
                      ))}
                    </div>
                  )}
                </div>

              </>
            )}
          </>
        ) : currentPage === 'control' ? (
          // CONTROL PAGE
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
                  <div className="flex items-center justify-between">
                    <h2 className="text-lg font-semibold flex items-center gap-2">
                      <span>📟</span>
                      System Output
                    </h2>
                    <button
                      onClick={() => setState(prev => ({ ...prev, output: [] }))}
                      className={`px-3 py-1 rounded-lg text-xs font-medium transition-colors ${
                        darkMode ? 'bg-gray-700 hover:bg-gray-600 text-gray-300' : 'bg-gray-200 hover:bg-gray-300 text-gray-700'
                      }`}
                    >
                      Clear Output
                    </button>
                  </div>
                </div>
                <div className={`p-4 h-[600px] overflow-y-auto ${darkMode ? 'bg-gray-900' : 'bg-gray-50'}`}>
                  {state.output.length === 0 ? (
                    <div className="text-center py-12">
                      <div className="text-6xl mb-4 opacity-50">📟</div>
                      <div className="text-gray-500 font-medium">No output yet</div>
                      <div className="text-gray-600 text-sm mt-2">Start the fleet to see system messages</div>
                    </div>
                  ) : (
                    <div className="space-y-2">
                      {state.output.map((line, i) => {
                        const style = getMessageStyle(line);
                        const isTableLine = line.includes('╔') || line.includes('║') || line.includes('╚') || line.includes('═') || line.includes('╠') || line.includes('╣');
                        
                        if (isTableLine) {
                          return (
                            <div key={i} className={`${style.bg} ${style.border} rounded-lg p-3 font-mono text-xs overflow-x-auto`}>
                              <pre className={`${style.color} whitespace-pre`}>{line}</pre>
                            </div>
                          );
                        }
                        
                        return (
                          <div key={i} className={`${style.bg} ${style.border} rounded-lg px-4 py-2 font-mono text-xs flex items-start gap-3 transition-all hover:scale-[1.01]`}>
                            <span className="text-lg leading-none mt-0.5">{style.icon}</span>
                            <span className={`${style.color} flex-1 break-all`}>{line}</span>
                          </div>
                        );
                      })}
                    </div>
                  )}
                  <div ref={outputEndRef} />
                </div>
              </div>
            </div>
          </>
        ) : currentPage === 'robots' ? (
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
        ) : null}
        </main>
      </div>
    </div>
  );
}

export default App;
