import { useFleetStore } from '../stores/fleetStore';

export default function ChargingStations() {
  // Use chargingStations directly from robots.json (already parsed in store)
  const chargingStations = useFleetStore(state => state.chargingStations);

  if (!chargingStations || chargingStations.length === 0) {
    return (
      <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
        <h3 className="text-lg font-bold text-gray-100 mb-4 flex items-center gap-2">
          <span>🔌</span>
          Charging Stations
        </h3>
        <div className="text-center py-8 text-gray-400">
          <div className="text-4xl mb-2">⚡</div>
          <p className="text-sm">No charging stations configured</p>
        </div>
      </div>
    );
  }

  return (
    <div className="bg-gray-800 border border-gray-700 rounded-2xl shadow-lg p-6">
      <h3 className="text-lg font-bold text-gray-100 mb-4 flex items-center gap-2">
        <span>🔌</span>
        Charging Stations
        <span className="ml-auto text-sm font-normal text-gray-400">
          {chargingStations.filter(s => s.status === 'AVAILABLE').length}/{chargingStations.length} Free
        </span>
      </h3>

      {/* Vertical list layout to prevent overlap */}
      <div className="space-y-3">
        {chargingStations.map((station) => (
          <div
            key={station.id}
            className={`p-4 rounded-lg border transition-all ${station.status === 'OCCUPIED'
                ? 'bg-green-900/20 border-green-700/50'
                : 'bg-blue-900/20 border-blue-700/50'
              }`}
          >
            {/* Main row with flex-wrap for safety */}
            <div className="flex items-center gap-3">
              {/* Icon */}
              <span className="text-2xl flex-shrink-0">
                {station.status === 'OCCUPIED' ? '🔋' : '⚡'}
              </span>

              {/* Station info - grows to fill */}
              <div className="flex-1 min-w-0">
                <h4 className="font-semibold text-gray-100 truncate">
                  Station C{station.id}
                </h4>
                <p className="text-xs text-gray-400">
                  Position: ({station.x}, {station.y})
                </p>
              </div>

              {/* Status badge - fixed width, won't overlap */}
              <div
                className={`px-3 py-1.5 rounded-full text-xs font-bold flex-shrink-0 ${station.status === 'OCCUPIED'
                    ? 'bg-green-500/20 text-green-400'
                    : 'bg-blue-500/20 text-blue-400'
                  }`}
              >
                {station.status === 'OCCUPIED' ? 'Charging' : 'Free'}
              </div>
            </div>

            {/* Robot info when occupied */}
            {station.status === 'OCCUPIED' && station.robot !== null && (
              <div className="mt-3 pt-3 border-t border-gray-700">
                <div className="flex items-center gap-2 text-sm text-gray-400">
                  <span>🤖</span>
                  <span>Robot #{station.robot} is charging</span>
                </div>
              </div>
            )}
          </div>
        ))}
      </div>
    </div>
  );
}
