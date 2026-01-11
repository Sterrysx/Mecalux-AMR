import { useState, useEffect, useRef, useMemo, Component, ReactNode } from 'react';
import { useFleetStore, useFleetOverview, useTaskStats } from './stores/fleetStore';
import { fleetAPI } from './services/FleetAPI';
import ChargingStations from './components/ChargingStations';
import TaskCompletionChart from './components/TaskCompletionChart';

// Error Boundary Component
class ErrorBoundary extends Component<{ children: ReactNode }, { hasError: boolean }> {
  constructor(props: { children: ReactNode }) {
    super(props);
    this.state = { hasError: false };
  }

  static getDerivedStateFromError() {
    return { hasError: true };
  }

  componentDidCatch(error: Error, errorInfo: any) {
    console.error('ErrorBoundary caught:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="bg-gray-800 border border-red-700 rounded-2xl shadow-lg p-6">
          <div className="text-center text-red-400">
            <div className="text-4xl mb-2">⚠️</div>
            <p>Component failed to load</p>
          </div>
        </div>
      );
    }
    return this.props.children;
  }
}

interface Robot {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  state: string;
  goal: string | null; // Node ID from backend
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
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
  const [selectedRobotId, setSelectedRobotId] = useState<number | null>(null);

  // Zustand store selectors
  const {
    updateRobots,
    updateTasks,
    updateMap,
    setPOIs,
    setBackendConnected,
    startSystem,
    stopSystem,
    updateUptime,
    addTaskHistoryPoint,
    uptime: systemUptime
  } = useFleetStore();

  // Direct selectors to avoid re-render loops
  const totalRobots = useFleetStore(state => state?.robots?.size || 0);
  // Subscribe to robots map for live updates in robots page
  const robotsMap = useFleetStore(state => state?.robots);
  const robotsList = Array.from(robotsMap?.values() || []);
  const activeRobots = useFleetStore(state => state?.getActiveRobotCount?.() || 0);
  const idleRobots = useFleetStore(state => state?.getIdleRobotCount?.() || 0);
  const avgBattery = useFleetStore(state => state?.getAverageBattery?.() || 100);
  const taskCompleted = useFleetStore(state => state?.getCompletedTaskCount?.() || 0);
  const totalTasks = useFleetStore(state => state?.tasks?.size || 0);
  const inProgressTasks = useFleetStore(state => state?.getInProgressTaskCount?.() || 0);
  const efficiency = useFleetStore(state => state?.getEfficiency?.() || 0);
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

  // Initialize Fleet API polling on mount - ONLY ONCE
  useEffect(() => {
    let mounted = true;

    // Check backend health
    fleetAPI.healthCheck().then(isHealthy => {
      if (mounted) {
        setBackendConnected(isHealthy);
        if (!isHealthy) {
          console.warn('Backend is not responding');
        }
      }
    }).catch(() => {
      if (mounted) setBackendConnected(false);
    });

    // Load POIs (one-time) - fail silently
    fleetAPI.fetchPOIs()
      .then(pois => {
        if (mounted && pois && pois.length > 0) {
          setPOIs(pois);
        }
      })
      .catch(error => {
        // Silently fail - POIs are optional
        console.warn('POIs not available (optional):', error.message);
      });

    // Start polling robots, tasks, and map data
    fleetAPI.startPolling({
      onRobotsUpdate: updateRobots,
      onTasksUpdate: updateTasks,
      onMapUpdate: updateMap,
      onError: (error) => {
        // Don't update state on every error to avoid loops
        if (mounted && Math.random() < 0.1) { // Only 10% of errors
          console.error('Fleet API error:', error.message);
        }
      }
    });

    return () => {
      mounted = false;
      fleetAPI.stopPolling();
    };
  }, []); // Empty deps - run ONLY ONCE

  // Update uptime every second, task history every minute when system is running
  useEffect(() => {
    if (state.isRunning) {
      // Uptime updates every second
      const uptimeInterval = setInterval(() => {
        updateUptime();
      }, 1000);

      // Task history updates every minute for efficiency calculation
      const historyInterval = setInterval(() => {
        addTaskHistoryPoint();
      }, 60000); // Every 60 seconds (1 minute)

      // Add initial point immediately
      addTaskHistoryPoint();

      return () => {
        clearInterval(uptimeInterval);
        clearInterval(historyInterval);
      };
    }
  }, [state.isRunning, updateUptime, addTaskHistoryPoint]);

  // Helper function to categorize and style messages
  const getMessageStyle = (line: string) => {
    if (line.startsWith('>')) {
      return { icon: '▶', color: 'text-gray-300', bg: darkMode ? 'bg-zinc-800/50' : 'bg-gray-100', border: 'border-l-4 border-gray-500' };
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
      return { icon: '💻', color: 'text-gray-300', bg: darkMode ? 'bg-zinc-800/50' : 'bg-gray-50', border: 'border-l-4 border-gray-500' };
    }
    // Help command lines - indented lines starting with spaces (command descriptions)
    if (line.match(/^\s{3,}\w/) || line.includes('inject <N>') || line.includes('Available commands')) {
      return { icon: '📖', color: 'text-cyan-400', bg: darkMode ? 'bg-cyan-900/10' : 'bg-cyan-50', border: 'border-l-4 border-cyan-500' };
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

              // Split multi-line output into individual lines for proper formatting
              const lines = message.data.split('\n').filter((line: string) => line.trim() !== '');

              return {
                ...prev,
                output: [...prev.output, ...lines],
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
        isRunning: true,
        startTime: Date.now(),
        stats: {
          ...prev.stats,
          totalRobots: numRobots,
          idleRobots: numRobots
        }
      }));

      // Initialize Zustand store system tracking
      startSystem();
    }
  };

  const stopFleetManager = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'stop_simulator'
      }));
    }

    // Stop Zustand store system tracking
    stopSystem();
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

  // Task history is now managed by Zustand store

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
      case 'MOVING': return 'text-gray-300';
      case 'EXECUTING': return 'text-purple-400';
      case 'WAITING': return 'text-yellow-400';
      default: return 'text-gray-400';
    }
  };

  const getStateBadgeColor = (state: string) => {
    switch (state) {
      case 'IDLE': return 'bg-gray-700 text-gray-300';
      case 'MOVING': return 'bg-gray-600 text-white';
      case 'EXECUTING': return 'bg-purple-600 text-white';
      case 'WAITING': return 'bg-yellow-600 text-white';
      default: return 'bg-gray-700 text-gray-300';
    }
  };

  return (
    <div className={`min-h-screen ${darkMode ? 'bg-zinc-950 text-gray-100' : 'bg-gray-50 text-gray-900'}`}>
      {/* Sidebar Navigation - Fixed */}
      <aside className={`${sidebarCollapsed ? 'w-20' : 'w-64'} bg-slate-900 text-white fixed left-0 top-0 h-screen transition-all duration-300 shadow-2xl flex flex-col z-50`}>
        <div className="p-6 flex-1 flex flex-col">
          {/* Navigation Menu */}
          <nav className="space-y-2 mt-8">
            <button
              onClick={() => setCurrentPage('dashboard')}
              className={`w-full flex items-center ${sidebarCollapsed ? 'justify-center' : 'gap-3'} px-4 py-3 rounded-lg transition-all border-l-4 ${currentPage === 'dashboard'
                ? 'bg-slate-700 border-gray-400 text-white'
                : 'border-transparent text-gray-400 hover:bg-slate-800 hover:text-white'
                }`}
            >
              <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" />
              </svg>
              {!sidebarCollapsed && <span className="font-medium">Dashboard</span>}
            </button>

            <button
              onClick={() => setCurrentPage('robots')}
              className={`w-full flex items-center ${sidebarCollapsed ? 'justify-center' : 'gap-3'} px-4 py-3 rounded-lg transition-all border-l-4 ${currentPage === 'robots'
                ? 'bg-slate-700 border-gray-400 text-white'
                : 'border-transparent text-gray-400 hover:bg-slate-800 hover:text-white'
                }`}
            >
              <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z" />
              </svg>
              {!sidebarCollapsed && <span className="font-medium">Fleet</span>}
            </button>

            <button
              onClick={() => setCurrentPage('control')}
              className={`w-full flex items-center ${sidebarCollapsed ? 'justify-center' : 'gap-3'} px-4 py-3 rounded-lg transition-all border-l-4 ${currentPage === 'control'
                ? 'bg-slate-700 border-gray-400 text-white'
                : 'border-transparent text-gray-400 hover:bg-slate-800 hover:text-white'
                }`}
            >
              <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
              </svg>
              {!sidebarCollapsed && <span className="font-medium">Control</span>}
            </button>

            <button className={`w-full flex items-center ${sidebarCollapsed ? 'justify-center' : 'gap-3'} px-4 py-3 rounded-lg transition-all border-l-4 border-transparent text-gray-400 hover:bg-slate-800 hover:text-white`}>
              <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" />
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
              </svg>
              {!sidebarCollapsed && <span className="font-medium">Settings</span>}
            </button>
          </nav>
        </div>

        {/* Sidebar Toggle - Fixed at bottom */}
        <div className="p-6 border-t border-slate-800">
          <button
            onClick={() => setSidebarCollapsed(!sidebarCollapsed)}
            className="w-full bg-slate-800 hover:bg-slate-700 p-3 rounded-lg transition-all flex items-center justify-center text-gray-300"
          >
            {sidebarCollapsed ? (
              <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
              </svg>
            ) : (
              <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
              </svg>
            )}
          </button>
        </div>
      </aside>

      {/* Main Content - Fixed layout with proper overflow */}
      <div className={`${sidebarCollapsed ? 'ml-20' : 'ml-64'} transition-all duration-300 h-screen overflow-y-auto`}>
        {/* Top Bar - Sticky header */}
        <header className={`${darkMode ? 'bg-slate-800 border-b border-slate-700' : 'bg-white'} shadow-sm px-8 py-4 flex items-center justify-between sticky top-0 z-10 flex-shrink-0`}>
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

        <main className={`p-8 ${darkMode ? 'bg-slate-950' : 'bg-gray-100'}`}>
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
                        <div className="bg-gradient-to-br from-gray-600 to-gray-700 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                          📊
                        </div>
                        <div className="bg-gray-500/20 text-gray-400 px-3 py-1 rounded-full text-sm font-bold">
                          +{efficiency.toFixed(1)}/min
                        </div>
                      </div>
                      <p className={`text-sm mb-1 ${darkMode ? 'text-gray-400' : 'text-gray-500'}`}>Tasks Completed</p>
                      <p className={`text-3xl font-bold ${darkMode ? 'text-gray-100' : 'text-gray-800'}`}>{taskCompleted}</p>
                      <p className={`text-xs mt-2 ${darkMode ? 'text-gray-500' : 'text-gray-400'}`}>of {totalTasks} total tasks</p>
                    </div>

                    {/* Active Robots */}
                    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                      <div className="flex items-center justify-between mb-4">
                        <div className="bg-gradient-to-br from-orange-500 to-orange-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                          🤖
                        </div>
                        <div className="bg-orange-500/20 text-orange-400 px-3 py-1 rounded-full text-sm font-bold">
                          {activeRobots} Active
                        </div>
                      </div>
                      <p className="text-sm text-gray-400 mb-1">Fleet Status</p>
                      <p className="text-3xl font-bold text-gray-100">{totalRobots}</p>
                      <p className="text-xs text-gray-500 mt-2">{idleRobots} idle robots</p>
                    </div>

                    {/* Battery Health */}
                    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6 hover:shadow-xl transition-shadow">
                      <div className="flex items-center justify-between mb-4">
                        <div className="bg-gradient-to-br from-green-500 to-green-600 w-14 h-14 rounded-xl flex items-center justify-center text-white text-2xl shadow-lg">
                          🔋
                        </div>
                        <div className={`${avgBattery >= 80 ? 'bg-green-500/20 text-green-400' : avgBattery >= 50 ? 'bg-yellow-500/20 text-yellow-400' : 'bg-red-500/20 text-red-400'} px-3 py-1 rounded-full text-sm font-bold`}>
                          {avgBattery >= 80 ? '●●●' : avgBattery >= 50 ? '●●○' : '●○○'}
                        </div>
                      </div>
                      <p className="text-sm text-gray-400 mb-1">Avg Battery</p>
                      <p className="text-3xl font-bold text-gray-100">{avgBattery}%</p>
                      <div className="w-full bg-gray-700 rounded-full h-2 mt-3">
                        <div
                          className={`h-2 rounded-full transition-all duration-500 ${getBatteryBgColor(avgBattery)}`}
                          style={{ width: `${avgBattery}%` }}
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
                      <p className="text-3xl font-bold text-gray-100">{efficiency > 0 ? efficiency.toFixed(1) : '0.0'}</p>
                      <p className="text-xs text-gray-500 mt-2">Uptime: {Math.floor((systemUptime || 0) / 60)}m {Math.floor((systemUptime || 0) % 60)}s</p>
                    </div>
                  </div>

                  {/* Main Dashboard Grid */}
                  <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-6">
                    {/* Task Completion Chart */}
                    <div className="lg:col-span-2">
                      <TaskCompletionChart />
                    </div>

                    {/* Charging Stations */}
                    <div>
                      <ErrorBoundary>
                        <ChargingStations />
                      </ErrorBoundary>
                    </div>
                  </div>

                </>
              )}
            </>
          ) : currentPage === 'control' ? (
            // CONTROL PAGE
            <>
              {/* Top Section - Conditional: Setup Form OR Stats Grid */}
              {!state.isRunning ? (
                // Fleet Initialization Form
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
                  {/* Fleet Configuration */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                    <div className="flex items-center gap-3 mb-6">
                      <span className="text-2xl">⚙️</span>
                      <h2 className="text-xl font-bold">Fleet Configuration</h2>
                    </div>

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
                          className={`w-full px-4 py-3 text-lg rounded-lg border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'}`}
                        />
                        <p className={`text-xs mt-2 ${darkMode ? 'text-gray-500' : 'text-gray-400'}`}>
                          Configure fleet size before starting (1-20 robots)
                        </p>
                      </div>

                      <button
                        onClick={startFleetManager}
                        disabled={!state.isConnected}
                        className="w-full px-6 py-4 text-lg bg-gradient-to-r from-green-500 to-green-600 text-white rounded-xl hover:from-green-600 hover:to-green-700 disabled:opacity-50 font-bold shadow-lg hover:shadow-xl transition-all flex items-center justify-center gap-3"
                      >
                        <span className="text-2xl">🚀</span>
                        <span>Start Fleet Manager</span>
                      </button>
                    </div>
                  </div>

                  {/* System Status */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                    <div className="flex items-center gap-3 mb-6">
                      <span className="text-2xl">📡</span>
                      <h2 className="text-xl font-bold">System Status</h2>
                    </div>

                    <div className="space-y-4">
                      <div className="flex items-center justify-between p-4 rounded-lg bg-gray-700/50">
                        <span className="text-sm font-medium">Backend Connection</span>
                        <div className="flex items-center gap-2">
                          <div className={`w-3 h-3 rounded-full ${state.isConnected ? 'bg-green-500 animate-pulse' : 'bg-red-500'}`} />
                          <span className="text-sm font-bold">{state.isConnected ? 'Connected' : 'Disconnected'}</span>
                        </div>
                      </div>

                      <div className="flex items-center justify-between p-4 rounded-lg bg-gray-700/50">
                        <span className="text-sm font-medium">Fleet Status</span>
                        <span className="text-sm font-bold text-gray-400">Not Started</span>
                      </div>

                      <div className="flex items-center justify-between p-4 rounded-lg bg-gray-700/50">
                        <span className="text-sm font-medium">Active Robots</span>
                        <span className="text-sm font-bold text-gray-400">0 / {numRobots}</span>
                      </div>
                    </div>
                  </div>
                </div>
              ) : (
                // Stats Grid (when running)
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
                  {/* Efficiency Card */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-gradient-to-br from-gray-50 to-gray-100 border-gray-200'}`}>
                    <div className="flex items-start justify-between">
                      <div>
                        <p className="text-sm font-medium text-gray-400">Efficiency</p>
                        <p className="text-3xl font-bold text-white mt-2">
                          {efficiency.toFixed(1)}
                        </p>
                        <p className="text-xs text-slate-200 mt-1">tasks/min</p>
                      </div>
                      <div className="text-3xl">📊</div>
                    </div>
                    <div className="mt-4 pt-4 border-t border-slate-500/50">
                      <p className="text-xs text-slate-200">
                        {taskCompleted} / {totalTasks} tasks completed
                      </p>
                    </div>
                  </div>

                  {/* Robots Card */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-gradient-to-br from-purple-50 to-purple-100 border-purple-200'}`}>
                    <div className="flex items-start justify-between">
                      <div>
                        <p className="text-sm font-medium text-gray-400">Fleet Status</p>
                        <p className="text-3xl font-bold text-white mt-2">
                          {activeRobots}/{totalRobots}
                        </p>
                        <p className="text-xs text-gray-400 mt-1">robots active</p>
                      </div>
                      <div className="text-3xl">🤖</div>
                    </div>
                    <div className="mt-4 pt-4 border-t border-gray-700">
                      <p className="text-xs text-gray-400">
                        {idleRobots} idle · {inProgressTasks} tasks running
                      </p>
                    </div>
                  </div>

                  {/* Battery Card */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-gradient-to-br from-green-50 to-green-100 border-green-200'}`}>
                    <div className="flex items-start justify-between">
                      <div>
                        <p className="text-sm font-medium text-gray-400">Avg Battery</p>
                        <p className="text-3xl font-bold text-white mt-2">
                          {avgBattery}%
                        </p>
                        <p className="text-xs text-gray-400 mt-1">fleet average</p>
                      </div>
                      <div className="text-3xl">🔋</div>
                    </div>
                    <div className="mt-4 pt-4 border-t border-gray-700">
                      <div className="w-full bg-gray-700/50 rounded-full h-2">
                        <div
                          className={`h-2 rounded-full transition-all ${getBatteryBgColor(avgBattery)}`}
                          style={{ width: `${avgBattery}%` }}
                        />
                      </div>
                    </div>
                  </div>

                  {/* Uptime Card */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-gradient-to-br from-orange-50 to-orange-100 border-orange-200'}`}>
                    <div className="flex items-start justify-between">
                      <div>
                        <p className="text-sm font-medium text-gray-400">Uptime</p>
                        <p className="text-3xl font-bold text-white mt-2">
                          {Math.floor(systemUptime / 60)}m
                        </p>
                        <p className="text-xs text-gray-400 mt-1">{Math.floor(systemUptime % 60)}s elapsed</p>
                      </div>
                      <div className="text-3xl">⏱️</div>
                    </div>
                    <div className="mt-4 pt-4 border-t border-gray-700">
                      <p className="text-xs text-gray-400">
                        {state.isRunning ? 'System operational' : 'System offline'}
                      </p>
                    </div>
                  </div>
                </div>
              )}

              {/* Bottom Section - Always visible */}
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                {/* Left Column: Controls */}
                <div className="space-y-6">
                  {/* Quick Actions */}
                  <div className={`rounded-xl border p-6 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                    <h2 className="text-lg font-semibold mb-4 flex items-center gap-2">
                      <span>⚡</span>
                      Quick Actions
                    </h2>
                    <div className="grid grid-cols-2 gap-3">
                      <button onClick={() => sendQuickCommand('inject 3')} disabled={!state.isRunning}
                        className="px-4 py-3 text-sm bg-zinc-700 hover:bg-zinc-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
                        Inject 3 Tasks<span className="block text-xs opacity-75">Scenario B</span>
                      </button>
                      <button onClick={() => sendQuickCommand('inject 5')} disabled={!state.isRunning}
                        className="px-4 py-3 text-sm bg-zinc-700 hover:bg-zinc-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
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
                            className="px-6 py-2 text-sm bg-zinc-700 hover:bg-zinc-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed font-medium transition-colors">
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
                        className={`px-3 py-1 rounded-lg text-xs font-medium transition-colors ${darkMode ? 'bg-gray-700 hover:bg-gray-600 text-gray-300' : 'bg-gray-200 hover:bg-gray-300 text-gray-700'
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
                          const isHelpLine = line.match(/^\s{3,}\w/) || line.includes('inject <N>') || line.includes('Available commands');

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
                              <span className={`${style.color} flex-1 ${isHelpLine ? 'whitespace-pre-wrap' : 'break-all'}`}>{line}</span>
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
              <div className="mb-6 flex items-center justify-between">
                <div>
                  <h2 className="text-2xl font-bold">Robot Fleet Monitor</h2>
                  <p className={`text-sm ${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
                    Real-time status, battery levels, and position tracking. Click a robot to view task history.
                  </p>
                </div>
                <button
                  onClick={() => {
                    console.log('🔄 Manually refreshing robot data...');
                    const btn = event?.target as HTMLButtonElement;
                    if (btn) {
                      btn.disabled = true;
                      btn.style.opacity = '0.6';
                    }

                    fleetAPI.stopPolling();
                    setTimeout(() => {
                      fleetAPI.startPolling({
                        onRobotsUpdate: (data) => {
                          console.log('✅ Fresh data received:', data.robots.length, 'robots, tick', data.tick);
                          updateRobots(data);
                          if (btn) {
                            btn.disabled = false;
                            btn.style.opacity = '1';
                          }
                        },
                        onTasksUpdate: updateTasks,
                        onMapUpdate: updateMap,
                        onError: (error) => {
                          console.error('Fleet API error:', error);
                          setBackendConnected(false);
                          if (btn) {
                            btn.disabled = false;
                            btn.style.opacity = '1';
                          }
                        }
                      });
                    }, 100);
                  }}
                  className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg font-medium transition-colors shadow-lg disabled:cursor-not-allowed"
                >
                  <svg className="w-5 h-5 animate-spin-on-click" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                  </svg>
                  Refresh
                </button>
              </div>

              {!state.isRunning ? (
                <div className={`rounded-xl border p-12 text-center ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                  <div className="text-6xl mb-4">🤖</div>
                  <h3 className="text-xl font-semibold mb-3">No Robots Configured</h3>
                  <p className={`mb-6 ${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
                    The fleet has not been initialized yet. Start the fleet manager to see robots.
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
              ) : totalRobots === 0 ? (
                <div className={`rounded-xl border p-12 text-center ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                  <div className="text-6xl mb-4 animate-pulse">⏳</div>
                  <h3 className="text-xl font-semibold mb-2">Loading Robots...</h3>
                  <p className={`${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
                    Fetching robot data from backend...
                  </p>
                </div>
              ) : (
                /* ROBOT GRID LAYOUT */
                <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
                  {robotsList.map((robot, idx) => {
                    const batteryLevel = robot?.batteryLevel ?? 100;
                    const state = robot?.state || 'IDLE';

                    // State-based card styling
                    const getCardStyle = (s: string) => {
                      switch (s) {
                        case 'MOVING': return 'border-blue-500/50 bg-gradient-to-br from-blue-900/20 to-gray-800';
                        case 'COMPUTING_PATH': return 'border-purple-500/50 bg-gradient-to-br from-purple-900/20 to-gray-800';
                        case 'ARRIVED': return 'border-green-500/50 bg-gradient-to-br from-green-900/20 to-gray-800';
                        case 'COLLISION_WAIT': return 'border-yellow-500/50 bg-gradient-to-br from-yellow-900/20 to-gray-800';
                        case 'CARRYING': return 'border-orange-500/50 bg-gradient-to-br from-orange-900/20 to-gray-800';
                        default: return 'border-gray-600 bg-gray-800';
                      }
                    };

                    const getStatePulse = (s: string) => {
                      switch (s) {
                        case 'MOVING': return 'bg-blue-500 animate-pulse';
                        case 'COMPUTING_PATH': return 'bg-purple-500 animate-pulse';
                        case 'ARRIVED': return 'bg-green-500';
                        case 'COLLISION_WAIT': return 'bg-yellow-500 animate-pulse';
                        case 'CARRYING': return 'bg-orange-500';
                        default: return 'bg-gray-500';
                      }
                    };

                    return (
                      <div
                        key={robot?.id !== undefined ? robot.id : idx}
                        className={`rounded-xl border-2 p-5 transition-all hover:scale-[1.02] hover:shadow-xl cursor-pointer ${getCardStyle(state)}`}
                        onClick={() => setSelectedRobotId(robot?.id ?? null)}
                      >
                        {/* Header: Robot ID and Status Indicator */}
                        <div className="flex items-center justify-between mb-4">
                          <div className="flex items-center gap-3">
                            <div className="text-3xl">🤖</div>
                            <div>
                              <h3 className="text-xl font-bold text-white">Robot #{robot?.id ?? 0}</h3>
                              <div className="flex items-center gap-2 mt-1">
                                <div className={`w-2.5 h-2.5 rounded-full ${getStatePulse(state)}`}></div>
                                <span className="text-sm font-medium text-gray-300">{state}</span>
                              </div>
                            </div>
                          </div>
                        </div>

                        {/* Position Info */}
                        <div className="bg-gray-900/50 rounded-lg p-3 mb-3">
                          <div className="flex items-center justify-between">
                            <span className="text-xs text-gray-400">📍 Position</span>
                            <span className="font-mono text-sm text-white">({robot?.x ?? 0}, {robot?.y ?? 0})</span>
                          </div>
                          {(robot?.vx !== 0 || robot?.vy !== 0) && (
                            <div className="flex items-center justify-between mt-1">
                              <span className="text-xs text-gray-400">⚡ Velocity</span>
                              <span className="font-mono text-xs text-gray-300">
                                ({(robot?.vx ?? 0).toFixed(1)}, {(robot?.vy ?? 0).toFixed(1)})
                              </span>
                            </div>
                          )}
                        </div>

                        {/* Battery Bar */}
                        <div className="mb-3">
                          <div className="flex items-center justify-between mb-1">
                            <span className="text-xs text-gray-400">🔋 Battery</span>
                            <span className={`text-sm font-bold ${getBatteryColor(batteryLevel)}`}>{batteryLevel}%</span>
                          </div>
                          <div className="w-full bg-gray-700 rounded-full h-2.5">
                            <div
                              className={`h-2.5 rounded-full transition-all duration-500 ${getBatteryBgColor(batteryLevel)}`}
                              style={{ width: `${batteryLevel}%` }}
                            />
                          </div>
                        </div>

                        {/* Task Info */}
                        <div className="bg-gray-900/50 rounded-lg p-3">
                          {robot?.goal !== null && robot?.goal !== undefined ? (
                            <div className="flex items-center gap-2">
                              <span className="text-yellow-400">🎯</span>
                              <span className="text-sm text-white">Goal: Node {robot.goal}</span>
                            </div>
                          ) : robot?.itinerary && robot.itinerary > 0 ? (
                            <div className="flex items-center gap-2">
                              <span className="text-purple-400">📋</span>
                              <span className="text-sm text-white">{robot.itinerary} waypoint{robot.itinerary > 1 ? 's' : ''} queued</span>
                            </div>
                          ) : (
                            <div className="flex items-center gap-2">
                              <span className="text-gray-500">💤</span>
                              <span className="text-sm text-gray-400">Awaiting tasks</span>
                            </div>
                          )}
                        </div>
                      </div>
                    );
                  })}
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
