import { useFleetStore } from '../stores/fleetStore';

interface TaskHistoryPoint {
  timestamp: number;
  completed: number;
  active: number;
}

export default function TaskCompletionChart() {
  const taskHistory = useFleetStore((state: any) => state?.taskHistory as TaskHistoryPoint[] || []);
  const completedCount = useFleetStore((state: any) => state?.completedTaskIds?.size as number || 0);
  const activeCount = useFleetStore((state: any) => state?.getInProgressTaskCount?.() as number || 0);

  if (taskHistory.length === 0) {
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

  const maxValue = taskHistory && taskHistory.length > 0 
    ? Math.max(
        ...taskHistory.map((p: TaskHistoryPoint) => Math.max(p?.completed || 0, p?.active || 0)),
        1
      )
    : 1;

  return (
    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
      <div className="flex items-center justify-between mb-6">
        <h3 className="text-lg font-bold text-gray-100">Task Completion Over Time</h3>
        <div className="flex gap-3">
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
      
      {/* Bar Chart */}
      <div className="h-64 flex items-end justify-between gap-1">
        {taskHistory.slice(-20).map((point: TaskHistoryPoint, i: number) => {
          const completedHeight = (point.completed / maxValue) * 100;
          const activeHeight = (point.active / maxValue) * 100;
          
          return (
            <div key={i} className="flex-1 flex flex-col items-center gap-1">
              {/* Active Tasks Bar (Orange) */}
              <div 
                className="w-full bg-gradient-to-t from-orange-500 to-orange-400 rounded-t-lg transition-all duration-500 hover:from-orange-600 hover:to-orange-500" 
                style={{ 
                  height: `${activeHeight}%`, 
                  minHeight: activeHeight > 0 ? '4px' : '0px' 
                }}
                title={`Active: ${point.active}`}
              />
              {/* Completed Tasks Bar (Gray) */}
              <div 
                className="w-full bg-gradient-to-t from-gray-600 to-gray-500 rounded-t-lg transition-all duration-500 hover:from-gray-700 hover:to-gray-600" 
                style={{ 
                  height: `${completedHeight}%`, 
                  minHeight: completedHeight > 0 ? '4px' : '0px' 
                }}
                title={`Completed: ${point.completed}`}
              />
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
