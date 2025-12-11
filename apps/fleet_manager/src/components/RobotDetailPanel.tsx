// Robot Detail Panel - Shows individual robot information
import { useFleetStore } from '../stores/fleetStore';
import { fleetAPI } from '../services/FleetAPI';

export default function RobotDetailPanel() {
  const robots = useFleetStore(state => Array.from(state.robots.values()));

  if (robots.length === 0) {
    return (
      <div className="text-center py-8 text-gray-500">
        <div className="text-4xl mb-2">🤖</div>
        <div>No robots connected</div>
        <div className="text-xs mt-1">Waiting for backend data...</div>
      </div>
    );
  }

  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
      {robots.map(robot => (
        <RobotCard key={robot.id} robot={robot} />
      ))}
    </div>
  );
}

function RobotCard({ robot }: { robot: any }) {
  const stateColors = {
    IDLE: 'bg-gray-100 text-gray-700',
    MOVING: 'bg-green-100 text-green-700',
    CARRYING: 'bg-blue-100 text-blue-700',
    COMPUTING_PATH: 'bg-yellow-100 text-yellow-700',
    COLLISION_WAIT: 'bg-orange-100 text-orange-700',
    ARRIVED: 'bg-purple-100 text-purple-700'
  };

  const stateEmoji = {
    IDLE: '⚪',
    MOVING: '🟢',
    CARRYING: '🔵',
    COMPUTING_PATH: '🟡',
    COLLISION_WAIT: '🟠',
    ARRIVED: '🟣'
  };

  const handleSendToCharge = async () => {
    try {
      await fleetAPI.sendRobotToCharge(robot.id, 'C1');
      console.log(`Sent robot ${robot.id} to charging`);
    } catch (error) {
      console.error('Failed to send robot to charge:', error);
    }
  };

  const battery = robot.batteryLevel || 100;
  const batteryColor = battery > 60 ? 'bg-green-500' : battery > 30 ? 'bg-yellow-500' : 'bg-red-500';

  return (
    <div className="bg-white border border-gray-200 rounded-lg p-4 hover:shadow-md transition-shadow">
      {/* Header */}
      <div className="flex items-center justify-between mb-3">
        <div className="text-lg font-bold text-gray-900">Robot #{robot.id}</div>
        <div className={`px-2 py-1 rounded text-xs font-medium ${stateColors[robot.state as keyof typeof stateColors] || stateColors.IDLE}`}>
          {stateEmoji[robot.state as keyof typeof stateEmoji] || '⚪'} {robot.state}
        </div>
      </div>

      {/* Battery */}
      <div className="mb-3">
        <div className="flex justify-between text-xs text-gray-600 mb-1">
          <span>Battery</span>
          <span className="font-medium">{battery}%</span>
        </div>
        <div className="w-full h-2 bg-gray-200 rounded-full overflow-hidden">
          <div 
            className={`h-full ${batteryColor} transition-all`}
            style={{ width: `${battery}%` }}
          />
        </div>
      </div>

      {/* Position */}
      <div className="mb-3 text-xs">
        <div className="text-gray-600 mb-1">Position</div>
        <div className="font-mono text-gray-900">
          X: {robot.x.toFixed(0)}px, Y: {robot.y.toFixed(0)}px
        </div>
      </div>

      {/* Velocity */}
      {(robot.vx !== 0 || robot.vy !== 0) && (
        <div className="mb-3 text-xs">
          <div className="text-gray-600 mb-1">Velocity</div>
          <div className="font-mono text-gray-900">
            {Math.sqrt(robot.vx * robot.vx + robot.vy * robot.vy).toFixed(2)} m/s
          </div>
        </div>
      )}

      {/* Itinerary */}
      <div className="mb-3">
        <div className="text-xs text-gray-600 mb-1">Tasks in Queue</div>
        <div className="text-lg font-bold text-gray-900">
          {robot.itinerary?.length || 0}
        </div>
        {robot.goal && (
          <div className="text-xs text-gray-500 mt-1">
            Current goal: Node {robot.goal}
          </div>
        )}
      </div>

      {/* Actions */}
      <div className="flex gap-2 pt-3 border-t border-gray-200">
        <button
          onClick={handleSendToCharge}
          className="flex-1 py-1.5 px-2 text-xs font-medium bg-blue-50 text-blue-600 rounded hover:bg-blue-100 transition-colors"
        >
          ⚡ Charge
        </button>
        <button
          className="flex-1 py-1.5 px-2 text-xs font-medium bg-gray-50 text-gray-600 rounded hover:bg-gray-100 transition-colors"
        >
          📊 Details
        </button>
      </div>
    </div>
  );
}
