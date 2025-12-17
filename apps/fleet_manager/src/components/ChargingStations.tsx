import { useFleetStore } from '../stores/fleetStore';
import type { POI } from '../services/FleetAPI';

export default function ChargingStations() {
  const pois = useFleetStore((state: any) => state?.pois || []);
  
  // Calculate charging stations locally instead of in the store
  const chargingStations = pois.filter((p: POI) => p?.type === 'CHARGING');

  if (chargingStations.length === 0) {
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
      </h3>
      <div className="space-y-3">
        {chargingStations.map((station: POI & { occupied: boolean; robotId?: number; timeRemaining?: number }) => (
          <div
            key={station.id}
            className={`p-4 rounded-lg border transition-all ${
              station.occupied
                ? 'bg-yellow-900/20 border-yellow-700/50'
                : 'bg-green-900/20 border-green-700/50'
            }`}
          >
            <div className="flex items-center justify-between mb-2">
              <div className="flex items-center gap-2">
                <span className="text-2xl">
                  {station.occupied ? '🔋' : '⚡'}
                </span>
                <div>
                  <h4 className="font-semibold text-gray-100">
                    {station.id}
                  </h4>
                  <p className="text-xs text-gray-400">
                    Node {station.nodeId} • ({station.x}, {station.y})
                  </p>
                </div>
              </div>
              <div
                className={`px-3 py-1 rounded-full text-xs font-bold ${
                  station.occupied
                    ? 'bg-yellow-500/20 text-yellow-400'
                    : 'bg-green-500/20 text-green-400'
                }`}
              >
                {station.occupied ? 'Occupied' : 'Free'}
              </div>
            </div>
            
            {station.occupied && station.robotId && (
              <div className="mt-3 pt-3 border-t border-gray-700">
                <div className="flex items-center justify-between text-sm">
                  <span className="text-gray-400">
                    🤖 Robot #{station.robotId}
                  </span>
                  {station.timeRemaining !== undefined && (
                    <span className="text-yellow-400 font-mono">
                      ~{station.timeRemaining}s remaining
                    </span>
                  )}
                </div>
              </div>
            )}
          </div>
        ))}
      </div>
    </div>
  );
}
