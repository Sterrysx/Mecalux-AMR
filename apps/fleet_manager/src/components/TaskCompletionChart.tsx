import { useFleetStore } from '../stores/fleetStore';

interface TaskHistoryPoint {
  minute: number;
  tasksCompleted: number;
  tasksInProgress: number;
}

export default function TaskCompletionChart() {
  const timeStats = useFleetStore((state: any) => state?.timeStats as TaskHistoryPoint[] || []);
  const completedCount = timeStats.length > 0 ? timeStats[timeStats.length - 1]?.tasksCompleted || 0 : 0;
  const activeCount = timeStats.length > 0 ? timeStats[timeStats.length - 1]?.tasksInProgress || 0 : 0;

  if (timeStats.length === 0) {
    return (
      <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-bold text-gray-100">Task Completion Over Time</h3>
          <div className="flex gap-2">
            <div className="flex items-center gap-2 text-xs">
              <div className="w-3 h-3 bg-gray-500 rounded"></div>
              <span className="text-gray-400">Completed</span>
            </div>
            <div className="flex items-center gap-2 text-xs">
              <div className="w-3 h-3 bg-orange-400 rounded"></div>
              <span className="text-gray-400">Active</span>
            </div>
          </div>
        </div>
        
        <div className="h-64 flex items-center justify-center text-gray-400">
          <div className="text-center">
            <div className="text-4xl mb-2">📈</div>
            <p>Collecting data...</p>
            <p className="text-xs mt-1">Real-time Performance</p>
          </div>
        </div>
        
        <div className="mt-4 pt-4 border-t border-gray-700">
          <div className="flex justify-between text-sm text-gray-400">
            <span>Real-time Performance</span>
            <span className="font-semibold">{completedCount} tasks • 0.0/min</span>
          </div>
        </div>
      </div>
    );
  }

  const maxValue = timeStats && timeStats.length > 0 
    ? Math.max(
        ...timeStats.map((p: TaskHistoryPoint) => Math.max(p?.tasksCompleted || 0, p?.tasksInProgress || 0)),
        1
      )
    : 1;

  return (
    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
      <div className="flex items-center justify-between mb-6">
        <h3 className="text-lg font-bold text-gray-100">Tasques per Minut</h3>
        <div className="flex gap-3">
          <div className="flex items-center gap-2 text-xs">
            <div className="w-3 h-3 bg-emerald-500 rounded"></div>
            <span className="text-gray-400">Completades</span>
          </div>
          <div className="flex items-center gap-2 text-xs">
            <div className="w-3 h-3 bg-orange-400 rounded"></div>
            <span className="text-gray-400">En Progrés</span>
          </div>
        </div>
      </div>
      
      {/* Bar Chart */}
      <div className="h-64 flex items-end justify-between gap-1">
        {timeStats.slice(-20).map((point: TaskHistoryPoint, i: number) => {
          const completedHeight = (point.tasksCompleted / maxValue) * 100;
          const activeHeight = (point.tasksInProgress / maxValue) * 100;
          
          return (
            <div key={i} className="flex-1 flex flex-col-reverse items-center gap-1">
              {/* Completed Tasks Bar (Green) */}
              <div 
                className="w-full bg-gradient-to-t from-emerald-600 to-emerald-400 rounded-t-lg transition-all duration-500 hover:from-emerald-700 hover:to-emerald-500" 
                style={{ 
                  height: `${completedHeight}%`, 
                  minHeight: completedHeight > 0 ? '4px' : '0px' 
                }}
                title={`Minut ${point.minute}: Completades: ${point.tasksCompleted}`}
              />
              {/* Active Tasks Bar (Orange) */}
              <div 
                className="w-full bg-gradient-to-t from-orange-500 to-orange-400 rounded-t-lg transition-all duration-500 hover:from-orange-600 hover:to-orange-500" 
                style={{ 
                  height: `${activeHeight}%`, 
                  minHeight: activeHeight > 0 ? '4px' : '0px' 
                }}
                title={`Minut ${point.minute}: En progrés: ${point.tasksInProgress}`}
              />
              {/* Minute label */}
              <span className="text-[10px] text-gray-500 mt-1">{point.minute}</span>
            </div>
          );
        })}
      </div>
      
      <div className="mt-4 pt-4 border-t border-zinc-700">
        <div className="flex justify-between text-sm">
          <span className="text-gray-400">Real-time Performance</span>
          <span className="font-semibold text-gray-100">
            {completedCount} tasks • {activeCount} active
          </span>
        </div>
      </div>
    </div>
  );
}
