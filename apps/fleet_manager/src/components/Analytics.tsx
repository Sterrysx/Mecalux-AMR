// Analytics Page - Fleet Performance Metrics and Charts
import { useFleetStore } from '../stores/fleetStore';

interface PerformanceMetric {
    label: string;
    value: number;
    unit: string;
    trend?: 'up' | 'down' | 'stable';
    color: string;
}

export default function Analytics() {
    // Get store data
    const taskStats = useFleetStore(state => state.taskStats);
    const robots = useFleetStore(state => Array.from(state.robots.values()));
    const chargingStations = useFleetStore(state => state.chargingStations);
    const uptime = useFleetStore(state => state.uptime);
    const taskHistory = useFleetStore(state => state.taskHistory);

    // Calculate metrics
    const totalRobots = robots.length;
    const activeRobots = robots.filter(r => r.state !== 'IDLE' && r.state !== 'ARRIVED').length;
    const idleRobots = totalRobots - activeRobots;
    const avgBattery = totalRobots > 0
        ? Math.round(robots.reduce((sum, r) => sum + (r.batteryLevel || 100), 0) / totalRobots)
        : 100;

    const completedTasks = taskStats?.completed || 0;
    const activeTasks = taskStats?.active || 0;
    const pendingTasks = taskStats?.pending || 0;

    const availableStations = chargingStations.filter(s => s.status === 'AVAILABLE').length;
    const occupiedStations = chargingStations.filter(s => s.status === 'OCCUPIED').length;

    // Calculate efficiency (tasks per minute)
    const minutes = uptime / 60;
    const efficiency = minutes > 0 ? (completedTasks / minutes).toFixed(2) : '0.00';

    // Calculate fleet utilization (% of robots active)
    const utilization = totalRobots > 0 ? Math.round((activeRobots / totalRobots) * 100) : 0;

    // Format uptime
    const formatUptime = (seconds: number) => {
        const hrs = Math.floor(seconds / 3600);
        const mins = Math.floor((seconds % 3600) / 60);
        const secs = Math.floor(seconds % 60);
        return `${hrs}h ${mins}m ${secs}s`;
    };

    const metrics: PerformanceMetric[] = [
        { label: 'Total Tasks Completed', value: completedTasks, unit: 'tasks', color: 'from-green-500 to-emerald-600' },
        { label: 'Active Tasks', value: activeTasks, unit: 'tasks', color: 'from-blue-500 to-cyan-600' },
        { label: 'Pending Tasks', value: pendingTasks, unit: 'queue', color: 'from-yellow-500 to-orange-600' },
        { label: 'Efficiency Rate', value: parseFloat(efficiency), unit: 'tasks/min', color: 'from-purple-500 to-pink-600' },
    ];

    return (
        <div className="space-y-6">
            {/* Header */}
            <div className="mb-6">
                <h2 className="text-2xl font-bold text-white">Analytics Dashboard</h2>
                <p className="text-sm text-gray-400">Real-time fleet performance metrics and historical data</p>
            </div>

            {/* Key Metrics Cards */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                {metrics.map((metric, idx) => (
                    <div key={idx} className="bg-gray-800 border border-gray-700 rounded-xl p-5 hover:shadow-lg transition-shadow">
                        <div className={`w-12 h-12 rounded-lg bg-gradient-to-br ${metric.color} flex items-center justify-center mb-4`}>
                            <span className="text-2xl text-white">
                                {idx === 0 ? '✅' : idx === 1 ? '🔄' : idx === 2 ? '⏳' : '📊'}
                            </span>
                        </div>
                        <p className="text-sm text-gray-400 mb-1">{metric.label}</p>
                        <p className="text-3xl font-bold text-white">{metric.value}</p>
                        <p className="text-xs text-gray-500 mt-1">{metric.unit}</p>
                    </div>
                ))}
            </div>

            {/* Fleet Status Section */}
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                {/* Robot Status */}
                <div className="bg-gray-800 border border-gray-700 rounded-xl p-6">
                    <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
                        🤖 Fleet Status
                    </h3>
                    <div className="space-y-4">
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Total Robots</span>
                            <span className="text-2xl font-bold text-white">{totalRobots}</span>
                        </div>
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Active</span>
                            <span className="text-xl font-semibold text-green-400">{activeRobots}</span>
                        </div>
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Idle</span>
                            <span className="text-xl font-semibold text-gray-500">{idleRobots}</span>
                        </div>

                        {/* Utilization Bar */}
                        <div className="pt-4 border-t border-gray-700">
                            <div className="flex items-center justify-between mb-2">
                                <span className="text-sm text-gray-400">Fleet Utilization</span>
                                <span className="text-sm font-bold text-white">{utilization}%</span>
                            </div>
                            <div className="w-full bg-gray-700 rounded-full h-3">
                                <div
                                    className="bg-gradient-to-r from-blue-500 to-cyan-400 h-3 rounded-full transition-all duration-500"
                                    style={{ width: `${utilization}%` }}
                                />
                            </div>
                        </div>
                    </div>
                </div>

                {/* Charging Stations */}
                <div className="bg-gray-800 border border-gray-700 rounded-xl p-6">
                    <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
                        ⚡ Charging Infrastructure
                    </h3>
                    <div className="space-y-4">
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Total Stations</span>
                            <span className="text-2xl font-bold text-white">{chargingStations.length}</span>
                        </div>
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Available</span>
                            <span className="text-xl font-semibold text-blue-400">{availableStations}</span>
                        </div>
                        <div className="flex items-center justify-between">
                            <span className="text-gray-400">Occupied</span>
                            <span className="text-xl font-semibold text-green-400">{occupiedStations}</span>
                        </div>

                        {/* Average Battery */}
                        <div className="pt-4 border-t border-gray-700">
                            <div className="flex items-center justify-between mb-2">
                                <span className="text-sm text-gray-400">Avg Fleet Battery</span>
                                <span className="text-sm font-bold text-white">{avgBattery}%</span>
                            </div>
                            <div className="w-full bg-gray-700 rounded-full h-3">
                                <div
                                    className={`h-3 rounded-full transition-all duration-500 ${avgBattery >= 80 ? 'bg-gradient-to-r from-green-500 to-emerald-400' :
                                            avgBattery >= 50 ? 'bg-gradient-to-r from-yellow-500 to-orange-400' :
                                                'bg-gradient-to-r from-red-500 to-pink-400'
                                        }`}
                                    style={{ width: `${avgBattery}%` }}
                                />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            {/* Task History Chart (Simple Visual) */}
            <div className="bg-gray-800 border border-gray-700 rounded-xl p-6">
                <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
                    📈 Task Completion Over Time
                </h3>

                {taskHistory.length > 0 ? (
                    <div className="h-48 flex items-end gap-1">
                        {taskHistory.slice(-30).map((point, idx) => {
                            const maxCompleted = Math.max(...taskHistory.map(p => p.completed), 1);
                            const height = (point.completed / maxCompleted) * 100;
                            return (
                                <div
                                    key={idx}
                                    className="flex-1 bg-gradient-to-t from-green-600 to-green-400 rounded-t hover:from-green-500 hover:to-green-300 transition-all cursor-pointer group relative"
                                    style={{ height: `${Math.max(height, 2)}%` }}
                                    title={`${point.completed} tasks completed`}
                                >
                                    <div className="absolute -top-8 left-1/2 -translate-x-1/2 bg-gray-900 px-2 py-1 rounded text-xs opacity-0 group-hover:opacity-100 transition-opacity whitespace-nowrap">
                                        {point.completed} tasks
                                    </div>
                                </div>
                            );
                        })}
                    </div>
                ) : (
                    <div className="h-48 flex items-center justify-center text-gray-500">
                        <div className="text-center">
                            <div className="text-4xl mb-2">📊</div>
                            <p>No historical data yet</p>
                            <p className="text-sm">Data will appear as tasks are completed</p>
                        </div>
                    </div>
                )}

                <div className="flex justify-between mt-4 text-xs text-gray-500">
                    <span>30 samples ago</span>
                    <span>Now</span>
                </div>
            </div>

            {/* System Info */}
            <div className="bg-gray-800 border border-gray-700 rounded-xl p-6">
                <h3 className="text-lg font-semibold text-white mb-4 flex items-center gap-2">
                    ⏱️ System Information
                </h3>
                <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                    <div className="text-center p-4 bg-gray-900/50 rounded-lg">
                        <p className="text-sm text-gray-400 mb-1">Uptime</p>
                        <p className="text-lg font-mono font-bold text-white">{formatUptime(uptime)}</p>
                    </div>
                    <div className="text-center p-4 bg-gray-900/50 rounded-lg">
                        <p className="text-sm text-gray-400 mb-1">Total Tasks</p>
                        <p className="text-lg font-mono font-bold text-white">{completedTasks + activeTasks + pendingTasks}</p>
                    </div>
                    <div className="text-center p-4 bg-gray-900/50 rounded-lg">
                        <p className="text-sm text-gray-400 mb-1">Efficiency</p>
                        <p className="text-lg font-mono font-bold text-green-400">{efficiency}/min</p>
                    </div>
                    <div className="text-center p-4 bg-gray-900/50 rounded-lg">
                        <p className="text-sm text-gray-400 mb-1">Station Usage</p>
                        <p className="text-lg font-mono font-bold text-blue-400">
                            {chargingStations.length > 0 ? Math.round((occupiedStations / chargingStations.length) * 100) : 0}%
                        </p>
                    </div>
                </div>
            </div>
        </div>
    );
}
