// Task Injector Component - UI for injecting new tasks
import { useState } from 'react';
import { fleetAPI } from '../services/FleetAPI';

export default function TaskInjector() {
  const [numTasks, setNumTasks] = useState(3);
  const [isInjecting, setIsInjecting] = useState(false);
  const [lastResult, setLastResult] = useState<any>(null);

  const handleInject = async () => {
    setIsInjecting(true);
    setLastResult(null);

    try {
      // Generate random tasks
      const tasks = Array.from({ length: numTasks }, () => ({
        pickup: `P${Math.floor(Math.random() * 30) + 1}`,
        dropoff: `D${Math.floor(Math.random() * 30) + 1}`
      }));

      const result = await fleetAPI.injectTasks(tasks);
      setLastResult(result);
      
      console.log('Tasks injected:', result);
    } catch (error) {
      console.error('Failed to inject tasks:', error);
      setLastResult({ error: 'Failed to inject tasks' });
    } finally {
      setIsInjecting(false);
    }
  };

  const scenario = numTasks <= 5 ? 'B' : 'C';
  const scenarioColor = scenario === 'B' ? 'text-green-600' : 'text-orange-600';
  const scenarioDesc = scenario === 'B' 
    ? 'Cheap Insertion (instant assignment)' 
    : 'Background Re-plan (starter tasks + async VRP)';

  return (
    <div className="bg-white rounded-lg shadow-sm p-4">
      <h3 className="text-sm font-semibold text-gray-900 mb-3">Task Injection</h3>
      
      <div className="space-y-4">
        {/* Number of tasks input */}
        <div>
          <label className="block text-xs font-medium text-gray-700 mb-2">
            Number of Tasks
          </label>
          <div className="flex items-center gap-2">
            <input
              type="range"
              min="1"
              max="20"
              value={numTasks}
              onChange={(e) => setNumTasks(parseInt(e.target.value))}
              className="flex-1"
            />
            <input
              type="number"
              min="1"
              max="20"
              value={numTasks}
              onChange={(e) => setNumTasks(parseInt(e.target.value))}
              className="w-16 px-2 py-1 border border-gray-300 rounded text-sm text-center"
            />
          </div>
        </div>

        {/* Scenario preview */}
        <div className={`p-3 rounded-lg bg-gray-50 border border-gray-200`}>
          <div className="flex items-start gap-2">
            <div className={`text-lg ${scenarioColor}`}>
              {scenario === 'B' ? '⚡' : '🔄'}
            </div>
            <div className="flex-1">
              <div className="text-xs font-semibold text-gray-900">
                Scenario {scenario}
              </div>
              <div className="text-xs text-gray-600 mt-0.5">
                {scenarioDesc}
              </div>
            </div>
          </div>
        </div>

        {/* Inject button */}
        <button
          onClick={handleInject}
          disabled={isInjecting}
          className={`w-full py-2 px-4 rounded-lg font-medium text-sm transition-colors ${
            isInjecting
              ? 'bg-gray-300 text-gray-500 cursor-not-allowed'
              : 'bg-zinc-700 text-white hover:bg-zinc-600'
          }`}
        >
          {isInjecting ? (
            <span className="flex items-center justify-center gap-2">
              <svg className="animate-spin h-4 w-4" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" fill="none" />
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
              </svg>
              Injecting...
            </span>
          ) : (
            `Inject ${numTasks} Task${numTasks !== 1 ? 's' : ''}`
          )}
        </button>

        {/* Result display */}
        {lastResult && (
          <div className={`p-3 rounded-lg text-xs ${
            lastResult.error 
              ? 'bg-red-50 border border-red-200 text-red-700'
              : 'bg-green-50 border border-green-200 text-green-700'
          }`}>
            {lastResult.error ? (
              <div>❌ {lastResult.error}</div>
            ) : (
              <div>
                <div className="font-semibold">✅ {lastResult.message}</div>
                <div className="mt-1 text-gray-600">
                  {lastResult.tasksInjected} tasks injected via Scenario {lastResult.scenario}
                </div>
              </div>
            )}
          </div>
        )}

        {/* Quick presets */}
        <div className="pt-3 border-t border-gray-200">
          <div className="text-xs font-medium text-gray-700 mb-2">Quick Presets</div>
          <div className="flex gap-2">
            <button
              onClick={() => setNumTasks(3)}
              className="flex-1 py-1 px-2 text-xs rounded border border-gray-300 hover:bg-gray-50"
            >
              3 tasks
            </button>
            <button
              onClick={() => setNumTasks(5)}
              className="flex-1 py-1 px-2 text-xs rounded border border-gray-300 hover:bg-gray-50"
            >
              5 tasks
            </button>
            <button
              onClick={() => setNumTasks(10)}
              className="flex-1 py-1 px-2 text-xs rounded border border-orange-300 bg-orange-50 hover:bg-orange-100"
            >
              10 tasks
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
