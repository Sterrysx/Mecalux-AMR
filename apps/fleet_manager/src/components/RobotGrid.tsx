import { useFleetStore } from '../stores/fleetStore';

const getRobotStateLabel = (robot: any) => {
  if (robot.currentTask && robot.currentTask !== -1) {
    return 'TREBALLANT';
  }
  if (robot.assignedTasks && robot.assignedTasks.length > 0) {
    return 'ASSIGNAT';
  }
  if (robot.state === 'MOVING' || robot.state === 'CARRYING') {
    return 'EN MOVIMENT';
  }
  if (robot.state === 'IDLE') {
    return 'INACTIU';
  }
  return robot.state || 'DESCONEGUT';
};

const getRobotStateBadgeColor = (state: string) => {
  switch (state) {
    case 'TREBALLANT':
      return 'bg-gradient-to-br from-emerald-500 to-emerald-600';
    case 'EN MOVIMENT':
      return 'bg-gradient-to-br from-blue-500 to-blue-600';
    case 'ASSIGNAT':
      return 'bg-gradient-to-br from-cyan-500 to-cyan-600';
    case 'INACTIU':
      return 'bg-gradient-to-br from-gray-500 to-gray-600';
    default:
      return 'bg-gradient-to-br from-gray-600 to-gray-700';
  }
};

const getBatteryColor = (battery: number) => {
  if (battery >= 70) return 'from-emerald-500 to-emerald-600';
  if (battery >= 40) return 'from-orange-500 to-orange-600';
  return 'from-red-500 to-red-600';
};

export default function RobotGrid() {
  const robots = useFleetStore((state) => Array.from(state.robots.values()));

  return (
    <div className="w-full">
      <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-bold text-gray-100">Estat de la Flota</h3>
          <span className="text-sm text-gray-400">{robots.length} robots</span>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 xl:grid-cols-5 gap-4">
          {robots.map((robot) => {
            const battery = robot.battery ?? robot.batteryLevel ?? 100;
            const state = getRobotStateLabel(robot);
            const currentTask = robot.currentTask !== undefined && robot.currentTask !== -1 
              ? `Tasca #${robot.currentTask}` 
              : 'Cap';
            const assignedCount = robot.assignedTasks?.length || 0;
            const completedCount = robot.completedTasks?.length || 0;

            return (
              <div
                key={robot.id}
                className="relative bg-gradient-to-br from-gray-900 to-gray-800 border border-gray-700 rounded-xl p-4 hover:border-gray-600 transition-all duration-300 hover:shadow-lg hover:shadow-gray-900/50"
              >
                {/* Robot ID Badge */}
                <div className="absolute -top-2 -right-2 w-8 h-8 bg-gradient-to-br from-blue-500 to-blue-600 rounded-full flex items-center justify-center text-white font-bold text-sm shadow-lg">
                  {robot.id}
                </div>

                {/* Status Badge */}
                <div className={`${getRobotStateBadgeColor(state)} text-white text-xs font-bold px-3 py-1 rounded-full mb-3 text-center shadow-md`}>
                  {state}
                </div>

                {/* Battery */}
                <div className="mb-3">
                  <div className="flex items-center justify-between mb-1">
                    <span className="text-xs text-gray-400">Bateria</span>
                    <span className="text-xs font-bold text-gray-200">{Math.round(battery)}%</span>
                  </div>
                  <div className="w-full h-2 bg-gray-700 rounded-full overflow-hidden">
                    <div
                      className={`h-full bg-gradient-to-r ${getBatteryColor(battery)} transition-all duration-500`}
                      style={{ width: `${battery}%` }}
                    />
                  </div>
                </div>

                {/* Current Task */}
                <div className="mb-3 pb-3 border-b border-gray-700">
                  <span className="text-xs text-gray-400 block mb-1">Tasca Actual</span>
                  <span className="text-sm font-semibold text-gray-200">{currentTask}</span>
                </div>

                {/* Task Stats */}
                <div className="grid grid-cols-2 gap-2">
                  <div className="text-center">
                    <div className="text-xs text-gray-400 mb-1">Per Fer</div>
                    <div className="text-lg font-bold text-orange-400">{assignedCount}</div>
                  </div>
                  <div className="text-center">
                    <div className="text-xs text-gray-400 mb-1">Fetes</div>
                    <div className="text-lg font-bold text-emerald-400">{completedCount}</div>
                  </div>
                </div>
              </div>
            );
          })}
        </div>

        {robots.length === 0 && (
          <div className="text-center py-12 text-gray-400">
            <div className="text-4xl mb-2">🤖</div>
            <p>No hi ha robots disponibles</p>
          </div>
        )}
      </div>
    </div>
  );
}
