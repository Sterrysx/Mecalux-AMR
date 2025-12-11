// Fleet Dashboard - Main component integrating all widgets
import { useEffect } from 'react';
import { fleetAPI } from '../services/FleetAPI';
import { useFleetStore, useFleetOverview, useTaskStats, useSystemStats } from '../stores/fleetStore';
import WarehouseCanvas from '../components/WarehouseCanvas';
import TaskInjector from '../components/TaskInjector';
import RobotDetailPanel from '../components/RobotDetailPanel';

export default function FleetDashboard() {
  const { 
    updateRobots, 
    updateTasks, 
    updateMap, 
    setPOIs, 
    setBackendConnected,
    updateSystemStats
  } = useFleetStore();
  
  const fleetOverview = useFleetOverview();
  const taskStats = useTaskStats();
  const systemStats = useSystemStats();

  // Initialize API polling
  useEffect(() => {
    // Check backend health
    fleetAPI.healthCheck().then(isHealthy => {
      setBackendConnected(isHealthy);
      if (!isHealthy) {
        console.warn('Backend is not responding');
      }
    });

    // Load POIs (one-time)
    fleetAPI.fetchPOIs().then(pois => {
      setPOIs(pois);
    });

    // Start polling
    fleetAPI.startPolling({
      onRobotsUpdate: updateRobots,
      onTasksUpdate: updateTasks,
      onMapUpdate: updateMap,
      onError: (error) => {
        console.error('Fleet API error:', error);
        setBackendConnected(false);
      }
    });

    // Fetch stats periodically
    const statsInterval = setInterval(() => {
      fleetAPI.getSystemStats().then(updateSystemStats);
    }, 2000);

    return () => {
      fleetAPI.stopPolling();
      clearInterval(statsInterval);
    };
  }, []);

  return (
    <div className="min-h-screen bg-gray-50">
      {/* Header */}
      <header className="bg-white shadow-sm border-b border-gray-200">
        <div className="max-w-7xl mx-auto px-6 py-4">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-2xl font-bold text-gray-900">Mecalux Fleet Manager</h1>
              <p className="text-sm text-gray-500">Real-time AMR coordination & monitoring</p>
            </div>
            <div className="flex items-center gap-4">
              <ConnectionStatus isConnected={systemStats.isBackendConnected} />
              <ScenarioIndicator scenario={systemStats.currentScenario} />
            </div>
          </div>
        </div>
      </header>

      <div className="max-w-7xl mx-auto px-6 py-6 space-y-6">
        {/* Stats Cards */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <StatsCard
            title="Total Robots"
            value={fleetOverview.totalRobots}
            subtitle={`${fleetOverview.activeRobots} active, ${fleetOverview.idleRobots} idle`}
            icon="🤖"
            color="blue"
          />
          <StatsCard
            title="Average Battery"
            value={`${fleetOverview.avgBattery}%`}
            subtitle={`${fleetOverview.chargingRobots} charging`}
            icon="🔋"
            color="green"
          />
          <StatsCard
            title="Task Queue"
            value={taskStats.pending + taskStats.assigned}
            subtitle={`${taskStats.inProgress} in progress`}
            icon="📦"
            color="yellow"
          />
          <StatsCard
            title="Completed"
            value={taskStats.completed}
            subtitle={`${systemStats.throughput || 0} tasks/min`}
            icon="✅"
            color="purple"
          />
        </div>

        {/* Main Content Grid */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Warehouse Visualization - Takes 2 columns */}
          <div className="lg:col-span-2">
            <div className="bg-white rounded-lg shadow-sm p-4">
              <h2 className="text-lg font-semibold text-gray-900 mb-4">Warehouse View</h2>
              <WarehouseCanvas width={900} height={500} className="w-full" />
            </div>
          </div>

          {/* Control Panel - Takes 1 column */}
          <div className="space-y-6">
            <TaskInjector />
            <PerformanceMetrics />
          </div>
        </div>

        {/* Robot Detail Panels */}
        <div className="bg-white rounded-lg shadow-sm p-4">
          <h2 className="text-lg font-semibold text-gray-900 mb-4">Robot Fleet</h2>
          <RobotDetailPanel />
        </div>
      </div>
    </div>
  );
}

// Connection Status Indicator
function ConnectionStatus({ isConnected }: { isConnected: boolean }) {
  return (
    <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-gray-100">
      <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`} />
      <span className="text-xs font-medium text-gray-700">
        {isConnected ? 'Connected' : 'Disconnected'}
      </span>
    </div>
  );
}

// Scenario Indicator
function ScenarioIndicator({ scenario }: { scenario?: 'A' | 'B' | 'C' }) {
  if (!scenario) return null;

  const scenarioInfo = {
    A: { label: 'Boot-Up', color: 'bg-blue-100 text-blue-700', desc: 'Full VRP Solve' },
    B: { label: 'Streaming', color: 'bg-green-100 text-green-700', desc: 'Cheap Insertion' },
    C: { label: 'Batch', color: 'bg-orange-100 text-orange-700', desc: 'Background Re-plan' }
  };

  const info = scenarioInfo[scenario];

  return (
    <div className={`px-3 py-1.5 rounded-full ${info.color}`}>
      <span className="text-xs font-medium">
        Scenario {scenario}: {info.label}
      </span>
    </div>
  );
}

// Stats Card Component
interface StatsCardProps {
  title: string;
  value: string | number;
  subtitle: string;
  icon: string;
  color: 'blue' | 'green' | 'yellow' | 'purple';
}

function StatsCard({ title, value, subtitle, icon, color }: StatsCardProps) {
  const colorClasses = {
    blue: 'bg-blue-50 text-blue-600',
    green: 'bg-green-50 text-green-600',
    yellow: 'bg-yellow-50 text-yellow-600',
    purple: 'bg-purple-50 text-purple-600'
  };

  return (
    <div className="bg-white rounded-lg shadow-sm p-6">
      <div className="flex items-start justify-between">
        <div>
          <p className="text-sm font-medium text-gray-600">{title}</p>
          <p className="text-3xl font-bold text-gray-900 mt-2">{value}</p>
          <p className="text-xs text-gray-500 mt-1">{subtitle}</p>
        </div>
        <div className={`text-3xl ${colorClasses[color]} p-3 rounded-lg`}>
          {icon}
        </div>
      </div>
    </div>
  );
}

// Performance Metrics Widget
function PerformanceMetrics() {
  const systemStats = useSystemStats();

  return (
    <div className="bg-white rounded-lg shadow-sm p-4">
      <h3 className="text-sm font-semibold text-gray-900 mb-3">Performance Metrics</h3>
      <div className="space-y-3 text-sm">
        <MetricRow 
          label="VRP Solve Time" 
          value={systemStats.lastVRPSolveTime ? `${systemStats.lastVRPSolveTime}ms` : 'N/A'} 
        />
        <MetricRow 
          label="Avg Path Query" 
          value={systemStats.avgPathQueryTime ? `${systemStats.avgPathQueryTime}ms` : 'N/A'} 
        />
        <MetricRow 
          label="Physics Loop" 
          value={systemStats.physicsLoopHz ? `${systemStats.physicsLoopHz} Hz` : 'N/A'} 
        />
        <MetricRow 
          label="Data Age" 
          value={`${Math.round(systemStats.dataAge / 1000)}s`} 
        />
        <MetricRow 
          label="VRP Solver" 
          value={systemStats.vrpSolverActive ? '🔴 Active' : '⚪ Idle'} 
        />
      </div>
    </div>
  );
}

function MetricRow({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex justify-between items-center">
      <span className="text-gray-600">{label}</span>
      <span className="font-mono font-medium text-gray-900">{value}</span>
    </div>
  );
}
