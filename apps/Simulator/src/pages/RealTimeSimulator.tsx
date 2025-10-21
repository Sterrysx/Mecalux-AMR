import { useState, useEffect, useRef } from 'react';
import { useTheme } from '../contexts/ThemeContext';

interface SimulatorState {
  isRunning: boolean;
  isConnected: boolean;
  output: string[];
}

interface TaskFormData {
  taskId: number;
  originNode: number;
  destinationNode: number;
}

export default function RealTimeSimulator() {
  const { darkMode } = useTheme();
  const [graphId, setGraphId] = useState(1);
  const [numRobots, setNumRobots] = useState(4);
  const [command, setCommand] = useState('');
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [taskForm, setTaskForm] = useState<TaskFormData>({
    taskId: 1,
    originNode: 1,
    destinationNode: 10
  });
  const [state, setState] = useState<SimulatorState>({
    isRunning: false,
    isConnected: false,
    output: []
  });
  
  const wsRef = useRef<WebSocket | null>(null);
  const outputEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    outputEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [state.output]);

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
              output: [...prev.output, message.message]
            }));
            break;
          case 'simulator_output':
            setState(prev => ({ 
              ...prev, 
              output: [...prev.output, message.data]
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

  const startSimulator = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'start_simulator',
        graphId,
        numRobots
      }));
      setState(prev => ({ ...prev, output: [] }));
    }
  };

  const stopSimulator = () => {
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

  const addTask = (e: React.FormEvent) => {
    e.preventDefault();
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      const cmd = `task ${taskForm.taskId} ${taskForm.originNode} ${taskForm.destinationNode}`;
      wsRef.current.send(JSON.stringify({
        type: 'send_command',
        command: cmd
      }));
      setState(prev => ({ 
        ...prev, 
        output: [...prev.output, 'Adding task ' + taskForm.taskId + ': Node ' + taskForm.originNode + ' to Node ' + taskForm.destinationNode]
      }));
      setTaskForm(prev => ({ ...prev, taskId: prev.taskId + 1 }));
    }
  };

  const sendQuickCommand = (cmd: string, description: string) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'send_command',
        command: cmd
      }));
      setState(prev => ({ 
        ...prev, 
        output: [...prev.output, '> ' + description]
      }));
    }
  };

  const clearOutput = () => {
    setState(prev => ({ ...prev, output: [] }));
  };

  return (
    <div className={`min-h-screen p-6 ${darkMode ? 'bg-gray-900 text-gray-100' : 'bg-gray-50 text-gray-900'}`}>
      <div className="max-w-7xl mx-auto space-y-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-2xl font-bold">Real-Time Simulator</h1>
            <p className={`text-xs mt-0.5 ${darkMode ? 'text-gray-400' : 'text-gray-600'}`}>
              Interactive warehouse robot simulation
            </p>
          </div>
          <div className="flex items-center gap-2">
            <div className={`w-2 h-2 rounded-full ${state.isConnected ? 'bg-green-500' : 'bg-red-500'}`} />
            <span className="text-xs">{state.isConnected ? 'Connected' : 'Disconnected'}</span>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <div className="space-y-4">
            <div className={`rounded-lg border p-4 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
              <h2 className="text-lg font-semibold mb-3">Configuration</h2>
              <div className="grid grid-cols-3 gap-3">
                <div>
                  <label className={`block text-xs font-medium mb-1 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>Graph ID</label>
                  <input type="number" min="1" max="10" value={graphId} onChange={(e) => setGraphId(Number(e.target.value))} disabled={state.isRunning}
                    className={`w-full px-2 py-1.5 text-sm rounded-md border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'} disabled:opacity-50`} />
                </div>
                <div>
                  <label className={`block text-xs font-medium mb-1 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>Robots</label>
                  <input type="number" min="1" max="10" value={numRobots} onChange={(e) => setNumRobots(Number(e.target.value))} disabled={state.isRunning}
                    className={`w-full px-2 py-1.5 text-sm rounded-md border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'} disabled:opacity-50`} />
                </div>
                <div className="flex items-end">
                  {!state.isRunning ? (
                    <button onClick={startSimulator} disabled={!state.isConnected}
                      className="w-full px-3 py-1.5 text-sm bg-green-600 text-white rounded-md hover:bg-green-700 disabled:opacity-50 font-medium">Start</button>
                  ) : (
                    <button onClick={stopSimulator} className="w-full px-3 py-1.5 text-sm bg-red-600 text-white rounded-md hover:bg-red-700 font-medium">Stop</button>
                  )}
                </div>
              </div>
            </div>

            {state.isRunning && (
              <div className={`rounded-lg border p-4 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
                <h2 className="text-lg font-semibold mb-3">Add Task</h2>
                <form onSubmit={addTask} className="space-y-3">
                  <div className="grid grid-cols-3 gap-3">
                    <div>
                      <label className={`block text-xs font-medium mb-1 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>Task ID</label>
                      <input type="number" min="1" value={taskForm.taskId} onChange={(e) => setTaskForm({ ...taskForm, taskId: Number(e.target.value) })}
                        className={`w-full px-2 py-1.5 text-sm rounded-md border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'}`} />
                    </div>
                    <div>
                      <label className={`block text-xs font-medium mb-1 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>Origin</label>
                      <input type="number" min="0" value={taskForm.originNode} onChange={(e) => setTaskForm({ ...taskForm, originNode: Number(e.target.value) })}
                        className={`w-full px-2 py-1.5 text-sm rounded-md border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'}`} />
                    </div>
                    <div>
                      <label className={`block text-xs font-medium mb-1 ${darkMode ? 'text-gray-300' : 'text-gray-700'}`}>Destination</label>
                      <input type="number" min="0" value={taskForm.destinationNode} onChange={(e) => setTaskForm({ ...taskForm, destinationNode: Number(e.target.value) })}
                        className={`w-full px-2 py-1.5 text-sm rounded-md border ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100' : 'bg-white border-gray-300 text-gray-900'}`} />
                    </div>
                  </div>
                  <button type="submit" className="w-full px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-700 font-medium">Add Task</button>

                  <div className="pt-3 border-t border-gray-700">
                    <div className="grid grid-cols-2 gap-2">
                      <button type="button" onClick={() => sendQuickCommand('status', 'Requesting status...')}
                        className={`px-3 py-1.5 rounded-md text-xs font-medium ${darkMode ? 'bg-gray-700 hover:bg-gray-600 text-gray-200' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'}`}>Status</button>
                      <button type="button" onClick={() => sendQuickCommand('help', 'Showing help...')}
                        className={`px-3 py-1.5 rounded-md text-xs font-medium ${darkMode ? 'bg-gray-700 hover:bg-gray-600 text-gray-200' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'}`}>Help</button>
                    </div>
                  </div>

                  <div className="pt-3 border-t border-gray-700">
                    <button type="button" onClick={() => setShowAdvanced(!showAdvanced)} className="text-xs text-gray-500 hover:text-gray-400 flex items-center gap-1">
                      {showAdvanced ? '' : ''} Advanced
                    </button>
                    {showAdvanced && (
                      <div className="mt-2 flex gap-2">
                        <input type="text" value={command} onChange={(e) => setCommand(e.target.value)} placeholder="Raw command..."
                          onKeyDown={(e) => { if (e.key === 'Enter') { e.preventDefault(); sendCommand(e as any); } }}
                          className={`flex-1 px-2 py-1 rounded-md border text-xs ${darkMode ? 'bg-gray-700 border-gray-600 text-gray-100 placeholder-gray-400' : 'bg-white border-gray-300 text-gray-900 placeholder-gray-500'}`} />
                        <button type="button" onClick={sendCommand} className="px-3 py-1 bg-gray-600 text-white rounded-md hover:bg-gray-700 text-xs font-medium">Send</button>
                      </div>
                    )}
                  </div>
                </form>
              </div>
            )}

            <div className={`rounded-lg border p-4 ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`}>
              <h2 className="text-sm font-semibold mb-2">Quick Guide</h2>
              <ul className="text-xs space-y-1">
                <li> Configure graph and robots then click Start</li>
                <li> Add tasks using origin and destination nodes</li>
                <li> Task ID auto-increments after each addition</li>
                <li> Use Status button to check robot states</li>
              </ul>
            </div>
          </div>

          <div className={`rounded-lg border ${darkMode ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'}`} style={{ height: 'calc(100vh - 180px)' }}>
            <div className="flex items-center justify-between p-3 border-b border-gray-700">
              <h2 className="text-lg font-semibold">Output</h2>
              <button onClick={clearOutput} className={`px-2 py-1 text-xs rounded-md ${darkMode ? 'bg-gray-700 hover:bg-gray-600' : 'bg-gray-200 hover:bg-gray-300'}`}>Clear</button>
            </div>
            <div className={`p-3 font-mono text-xs overflow-auto ${darkMode ? 'bg-gray-900' : 'bg-gray-50'}`} style={{ height: 'calc(100% - 48px)' }}>
              {state.output.length === 0 ? (
                <div className="text-gray-500 italic text-sm">Start simulator to see output...</div>
              ) : (
                state.output.map((line, index) => (
                  <div key={index} className="whitespace-pre-wrap break-words mb-0.5">{line}</div>
                ))
              )}
              <div ref={outputEndRef} />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
