import { useFleetStore } from '../stores/fleetStore';
import { Task } from '../services/FleetAPI';

interface RobotTaskHistoryProps {
  robotId: number;
  onClose: () => void;
}

export default function RobotTaskHistory({ robotId, onClose }: RobotTaskHistoryProps) {
  const robot = useFleetStore((state: any) => state.getRobotById(robotId));
  const tasks = useFleetStore((state: any) => state.getTasksByRobot(robotId) as Task[]);
  const pois = useFleetStore((state: any) => state.pois as any[]);

  if (!robot) {
    return null;
  }

  const getPOIName = (nodeId: number) => {
    const poi = pois.find((p: any) => p.nodeId === nodeId);
    return poi ? poi.id : `Node ${nodeId}`;
  };

  const getStatusColor = (status: Task['status']) => {
    switch (status) {
      case 'COMPLETED':
        return 'bg-green-900/20 border-green-700 text-green-400';
      case 'IN_PROGRESS':
        return 'bg-blue-900/20 border-blue-700 text-blue-400';
      case 'ASSIGNED':
        return 'bg-yellow-900/20 border-yellow-700 text-yellow-400';
      case 'PENDING':
        return 'bg-gray-700/20 border-gray-600 text-gray-400';
      default:
        return 'bg-gray-700/20 border-gray-600 text-gray-400';
    }
  };

  const completedTasks = tasks.filter((t: Task) => t.status === 'COMPLETED');
  const activeTasks = tasks.filter((t: Task) => t.status === 'IN_PROGRESS' || t.status === 'ASSIGNED');

  return (
    <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50 p-4">
      <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-2xl max-w-2xl w-full max-h-[80vh] overflow-hidden flex flex-col">
        {/* Header */}
        <div className="p-6 border-b border-gray-700">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="bg-blue-600 w-12 h-12 rounded-xl flex items-center justify-center text-2xl">
                🤖
              </div>
              <div>
                <h3 className="text-xl font-bold text-gray-100">
                  Robot #{robotId}
                </h3>
                <p className="text-sm text-gray-400">
                  Task History & Status
                </p>
              </div>
            </div>
            <button
              onClick={onClose}
              className="text-gray-400 hover:text-gray-200 transition-colors p-2 hover:bg-gray-700 rounded-lg"
            >
              <span className="text-2xl">×</span>
            </button>
          </div>
        </div>

        {/* Robot Info */}
        <div className="p-6 border-b border-gray-700 bg-gray-900/50">
          <div className="grid grid-cols-3 gap-4">
            <div>
              <p className="text-xs text-gray-400 mb-1">Status</p>
              <p className="text-sm font-semibold text-gray-100">{robot.state}</p>
            </div>
            <div>
              <p className="text-xs text-gray-400 mb-1">Battery</p>
              <p className={`text-sm font-semibold ${
                (robot.batteryLevel || 100) >= 80 ? 'text-green-400' :
                (robot.batteryLevel || 100) >= 50 ? 'text-yellow-400' :
                'text-red-400'
              }`}>
                {robot.batteryLevel || 100}%
              </p>
            </div>
            <div>
              <p className="text-xs text-gray-400 mb-1">Position</p>
              <p className="text-sm font-mono text-gray-100">
                ({robot.x}, {robot.y})
              </p>
            </div>
          </div>
        </div>

        {/* Task Lists */}
        <div className="flex-1 overflow-y-auto p-6">
          {/* Active Tasks */}
          {activeTasks.length > 0 && (
            <div className="mb-6">
              <h4 className="text-sm font-semibold text-gray-300 mb-3 flex items-center gap-2">
                <span>⚡</span>
                Active Tasks ({activeTasks.length})
              </h4>
              <div className="space-y-2">
                {activeTasks.map((task: Task) => (
                  <div
                    key={task.id}
                    className={`p-4 rounded-lg border ${getStatusColor(task.status)}`}
                  >
                    <div className="flex items-center justify-between mb-2">
                      <span className="font-semibold">Task #{task.id}</span>
                      <span className="text-xs px-2 py-1 rounded-full bg-gray-900/50">
                        {task.status}
                      </span>
                    </div>
                    <div className="text-xs space-y-1">
                      <div className="flex items-center gap-2">
                        <span className="text-gray-500">📦 Pickup:</span>
                        <span>{getPOIName(task.sourceNode)}</span>
                      </div>
                      <div className="flex items-center gap-2">
                        <span className="text-gray-500">🎯 Dropoff:</span>
                        <span>{getPOIName(task.destinationNode)}</span>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Completed Tasks */}
          <div>
            <h4 className="text-sm font-semibold text-gray-300 mb-3 flex items-center gap-2">
              <span>✅</span>
              Completed Tasks ({completedTasks.length})
            </h4>
            {completedTasks.length === 0 ? (
              <div className="text-center py-8 text-gray-500">
                <div className="text-3xl mb-2">📋</div>
                <p className="text-sm">No completed tasks yet</p>
              </div>
            ) : (
              <div className="space-y-2">
                {completedTasks.map((task: Task) => (
                  <div
                    key={task.id}
                    className={`p-4 rounded-lg border ${getStatusColor(task.status)}`}
                  >
                    <div className="flex items-center justify-between mb-2">
                      <span className="font-semibold">Task #{task.id}</span>
                      <span className="text-xs px-2 py-1 rounded-full bg-green-900/50 text-green-400">
                        ✓ COMPLETED
                      </span>
                    </div>
                    <div className="text-xs space-y-1">
                      <div className="flex items-center gap-2">
                        <span className="text-gray-500">📦 Pickup:</span>
                        <span>{getPOIName(task.sourceNode)}</span>
                      </div>
                      <div className="flex items-center gap-2">
                        <span className="text-gray-500">🎯 Dropoff:</span>
                        <span>{getPOIName(task.destinationNode)}</span>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* No tasks at all */}
          {tasks.length === 0 && (
            <div className="text-center py-12 text-gray-500">
              <div className="text-5xl mb-3">📋</div>
              <p className="text-sm">No tasks assigned to this robot</p>
              <p className="text-xs mt-1 text-gray-600">Tasks will appear here once assigned</p>
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-gray-700 bg-gray-900/50">
          <button
            onClick={onClose}
            className="w-full px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors font-medium"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
}
