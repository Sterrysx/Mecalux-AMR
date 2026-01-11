import { useFleetStore } from '../stores/fleetStore';

// Configuration
const HISTORY_LIMIT = 20;  // Backend provides last 20 minutes

export default function TaskCompletionChart() {
  // Get data from store (populated from robots.json)
  const backendHistory = useFleetStore((state) => state.backendHistory);

  // Calculate dynamic Y-axis max from history
  const maxValue = backendHistory.length > 0
    ? Math.max(...backendHistory.map(p => p.completedDelta + p.activeCount), 1)
    : 10;

  // Round up to nice number for Y-axis labels
  const yAxisMax = Math.max(Math.ceil(maxValue / 5) * 5, 5);
  const yAxisMid = Math.floor(yAxisMax / 2);

  return (
    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-bold text-gray-100">Task Completion Over Time</h3>
        <div className="flex gap-3">
          <div className="flex items-center gap-2 text-xs">
            <div className="w-3 h-3 bg-gray-500 rounded"></div>
            <span className="text-gray-400">Completed/min</span>
          </div>
          <div className="flex items-center gap-2 text-xs">
            <div className="w-3 h-3 bg-orange-400 rounded"></div>
            <span className="text-gray-400">Active</span>
          </div>
        </div>
      </div>

      {/* Chart Container with Y-Axis Labels */}
      <div className="flex">
        {/* Y-Axis Labels */}
        <div className="flex flex-col justify-between text-xs text-gray-500 pr-2 w-8">
          <span className="text-right">{yAxisMax}</span>
          <span className="text-right">{yAxisMid}</span>
          <span className="text-right">0</span>
        </div>

        {/* Bar Chart Area */}
        <div className="flex-1 h-48 flex items-end gap-1 border-l border-b border-gray-600 pl-2 pb-1 relative">
          {/* Grid lines */}
          <div className="absolute inset-0 flex flex-col justify-between pointer-events-none pl-2">
            <div className="border-t border-gray-700/50 w-full"></div>
            <div className="border-t border-gray-700/50 w-full"></div>
            <div></div>
          </div>

          {/* Render bars from backend history */}
          {backendHistory.map((point, i) => {
            const completedHeight = (point.completedDelta / yAxisMax) * 100;
            const activeHeight = (point.activeCount / yAxisMax) * 100;

            return (
              <div
                key={i}
                className="flex-1 flex flex-col justify-end min-w-[16px] relative z-10"
                title={`${point.completedDelta} completed, ${point.activeCount} active`}
              >
                {/* Stacked bar */}
                <div className="w-full flex flex-col">
                  {/* Active (Orange) - TOP */}
                  {point.activeCount > 0 && (
                    <div
                      className="w-full bg-gradient-to-t from-orange-600 to-orange-400 rounded-t transition-all duration-300"
                      style={{ height: `${Math.max(activeHeight * 1.8, 6)}px` }}
                    />
                  )}
                  {/* Completed (Gray) - BOTTOM */}
                  {point.completedDelta > 0 && (
                    <div
                      className={`w-full bg-gradient-to-t from-gray-600 to-gray-400 transition-all duration-300 ${point.activeCount === 0 ? 'rounded-t' : ''}`}
                      style={{ height: `${Math.max(completedHeight * 1.8, 6)}px` }}
                    />
                  )}
                  {/* Empty bar placeholder */}
                  {point.completedDelta === 0 && point.activeCount === 0 && (
                    <div className="w-full bg-gray-700/50 rounded-t" style={{ height: '4px' }} />
                  )}
                </div>
              </div>
            );
          })}

          {/* Empty slots for remaining capacity */}
          {Array.from({ length: Math.max(0, HISTORY_LIMIT - backendHistory.length) }).map((_, i) => (
            <div key={`empty-${i}`} className="flex-1 min-w-[16px] flex items-end">
              <div className="w-full bg-gray-700/30 rounded-t" style={{ height: '2px' }} />
            </div>
          ))}
        </div>
      </div>

      {/* Footer */}
      <div className="flex justify-between mt-2 text-xs text-gray-500">
        <span>{backendHistory.length} min history</span>
        <span>Updates every minute • Max 20 min</span>
      </div>
    </div>
  );
}
