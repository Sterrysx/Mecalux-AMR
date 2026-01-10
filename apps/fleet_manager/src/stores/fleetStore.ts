// Zustand Store for Fleet Management State
import { create } from 'zustand';
import { RobotState, Task, DynamicObstacle, POI, SystemStats, FleetData, TaskData, MapData, ChargingStationInfo, TasksInfo } from '../services/FleetAPI';

interface FleetState {
  // Robot data
  robots: Map<number, RobotState>;
  lastRobotUpdate: number;

  // Task data
  tasks: Map<number, Task>;
  lastTaskUpdate: number;

  // Task history tracking (1-minute intervals)
  taskHistory: Array<{ timestamp: number; completed: number; active: number; completedDelta: number; efficiency: number }>;
  completedTaskIds: Set<number>;
  previousCompletedTasks: number; // For delta calculation

  // Map data
  dynamicObstacles: DynamicObstacle[];
  pois: POI[];
  chargingStations: ChargingStationInfo[];  // From robots.json
  taskStats: TasksInfo | null;              // From robots.json
  lastMapUpdate: number;

  // System state
  systemStats: SystemStats;
  isBackendConnected: boolean;
  errorCount: number;
  systemStartTime: number | null;
  uptime: number;

  // Actions
  updateRobots: (data: FleetData) => void;
  updateTasks: (data: TaskData) => void;
  updateMap: (data: MapData) => void;
  setPOIs: (pois: POI[]) => void;
  updateSystemStats: (stats: SystemStats) => void;
  setBackendConnected: (connected: boolean) => void;
  incrementErrorCount: () => void;
  resetErrorCount: () => void;
  startSystem: () => void;
  stopSystem: () => void;
  updateUptime: () => void;
  addTaskHistoryPoint: () => void;

  // Computed getters
  getActiveRobotCount: () => number;
  getIdleRobotCount: () => number;
  getChargingRobotCount: () => number;
  getAverageBattery: () => number;
  getPendingTaskCount: () => number;
  getAssignedTaskCount: () => number;
  getInProgressTaskCount: () => number;
  getCompletedTaskCount: () => number;
  getRobotById: (id: number) => RobotState | undefined;
  getTaskById: (id: number) => Task | undefined;
  getTasksByStatus: (status: Task['status']) => Task[];
  getRobotsByState: (state: RobotState['state']) => RobotState[];
  getChargingStations: () => Array<POI & { occupied: boolean; robotId?: number; timeRemaining?: number }>;
  getTasksByRobot: (robotId: number) => Task[];
  getEfficiency: () => number;
  getTotalTaskCount: () => number;
}

export const useFleetStore = create<FleetState>((set, get) => ({
  // Initial state
  robots: new Map(),
  lastRobotUpdate: 0,
  tasks: new Map(),
  lastTaskUpdate: 0,
  taskHistory: [],
  completedTaskIds: new Set(),
  previousCompletedTasks: 0,
  dynamicObstacles: [],
  pois: [],
  chargingStations: [],
  taskStats: null,
  lastMapUpdate: 0,
  systemStats: {},
  isBackendConnected: false,
  errorCount: 0,
  systemStartTime: null,
  uptime: 0,

  // Actions
  updateRobots: (data: FleetData) => {
    const robotsMap = new Map<number, RobotState>();

    data.robots.forEach(robot => {
      robotsMap.set(robot.id, robot);
    });

    // Also extract charging_stations and tasks from robots.json
    set({
      robots: robotsMap,
      lastRobotUpdate: data.timestamp || Date.now(),
      chargingStations: data.charging_stations || [],
      taskStats: data.tasks || null
    });
  },

  updateTasks: (data: TaskData) => {
    const tasksMap = new Map<number, Task>();
    const prevCompleted = get().completedTaskIds;
    const newCompleted = new Set(prevCompleted);

    data.tasks.forEach(task => {
      tasksMap.set(task.id, task);
      if (task.status === 'COMPLETED' && !prevCompleted.has(task.id)) {
        newCompleted.add(task.id);
      }
    });

    set({
      tasks: tasksMap,
      lastTaskUpdate: data.timestamp || Date.now(),
      completedTaskIds: newCompleted
    });
  },

  updateMap: (data: MapData) => {
    set({
      dynamicObstacles: data.obstacles,
      lastMapUpdate: data.timestamp || Date.now()
    });
  },

  setPOIs: (pois: POI[]) => {
    set({ pois });
  },

  updateSystemStats: (stats: SystemStats) => {
    set({ systemStats: stats });
  },

  setBackendConnected: (connected: boolean) => {
    set({ isBackendConnected: connected });
  },

  incrementErrorCount: () => {
    set(state => ({ errorCount: state.errorCount + 1 }));
  },

  resetErrorCount: () => {
    set({ errorCount: 0 });
  },

  startSystem: () => {
    set({
      systemStartTime: Date.now(),
      uptime: 0,
      taskHistory: [],
      completedTaskIds: new Set()
    });
  },

  stopSystem: () => {
    set({
      systemStartTime: null,
      uptime: 0
    });
  },

  updateUptime: () => {
    const startTime = get().systemStartTime;
    if (startTime) {
      set({ uptime: (Date.now() - startTime) / 1000 });
    }
  },

  addTaskHistoryPoint: () => {
    const taskStats = get().taskStats;
    const robots = get().robots;
    const previousCompleted = get().previousCompletedTasks;
    const history = get().taskHistory;

    // Get current counts from taskStats (from robots.json)
    const currentCompleted = taskStats?.completed || 0;
    const currentActive = taskStats?.active || 0;

    // Calculate delta (tasks completed since last point)
    const completedDelta = Math.max(0, currentCompleted - previousCompleted);

    // Calculate efficiency: (tasks completed in period) / (number of robots)
    const robotCount = robots?.size || 1;
    const efficiency = robotCount > 0 ? (completedDelta / robotCount) : 0;

    const newPoint = {
      timestamp: Date.now(),
      completed: currentCompleted,
      active: currentActive,
      completedDelta,
      efficiency
    };

    // Keep last 30 points (30 minutes of history at 1-minute intervals)
    const updatedHistory = [...history, newPoint].slice(-30);
    set({
      taskHistory: updatedHistory,
      previousCompletedTasks: currentCompleted
    });
  },

  // Computed getters
  getActiveRobotCount: () => {
    try {
      const robots = Array.from(get().robots?.values() || []);
      // Active = any state that's not IDLE or ARRIVED
      return robots.filter(r =>
        r?.state && r.state !== 'IDLE' && r.state !== 'ARRIVED'
      ).length || 0;
    } catch (error) {
      return 0;
    }
  },

  getIdleRobotCount: () => {
    try {
      const robots = Array.from(get().robots?.values() || []);
      return robots.filter(r => r?.state === 'IDLE').length || 0;
    } catch (error) {
      return 0;
    }
  },

  getChargingRobotCount: () => {
    try {
      const robots = Array.from(get().robots?.values() || []);
      return robots.filter(r => r?.state === 'IDLE' && r?.goal === null).length || 0;
    } catch (error) {
      return 0;
    }
  },

  getAverageBattery: () => {
    try {
      const robots = Array.from(get().robots?.values() || []);
      if (!robots || robots.length === 0) return 100;

      const totalBattery = robots.reduce((sum, r) => sum + (r?.batteryLevel || 100), 0);
      return Math.round(totalBattery / robots.length) || 100;
    } catch (error) {
      console.error('Error calculating average battery:', error);
      return 100;
    }
  },

  getCompletedTaskCount: () => {
    // Use taskStats from robots.json if available
    const taskStats = get().taskStats;
    if (taskStats) return taskStats.completed;
    return get().completedTaskIds?.size || 0;
  },

  getInProgressTaskCount: () => {
    // Use taskStats from robots.json if available
    const taskStats = get().taskStats;
    if (taskStats) return taskStats.active;
    return get().getTasksByStatus('IN_PROGRESS').length;
  },

  getPendingTaskCount: () => {
    // Use taskStats from robots.json if available
    const taskStats = get().taskStats;
    if (taskStats) return taskStats.pending;
    return get().getTasksByStatus('PENDING').length;
  },

  getAssignedTaskCount: () => {
    return get().getTasksByStatus('ASSIGNED').length;
  },

  getRobotById: (id: number) => {
    return get().robots.get(id);
  },

  getTaskById: (id: number) => {
    return get().tasks.get(id);
  },

  getTasksByStatus: (status: Task['status']) => {
    const tasks = Array.from(get().tasks.values());
    return tasks.filter(t => t.status === status);
  },

  getRobotsByState: (state: RobotState['state']) => {
    const robots = Array.from(get().robots.values());
    return robots.filter(r => r.state === state);
  },

  getChargingStations: () => {
    try {
      const state = get();
      const pois = state?.pois || [];
      const chargingPois = pois.filter(p => p?.type === 'CHARGING');
      const robots = Array.from(state?.robots?.values() || []);

      return chargingPois.map(poi => {
        // Find if any robot is at this charging station
        // A robot is at a charging station if:
        // 1. Its goal matches the charging station's nodeId (heading there or already there)
        // 2. Its position is close to the charging station (within 15 pixels ~ 1.5m)
        // 3. Its state is IDLE or ARRIVED (charging) or has low battery and is nearby
        const robotAtStation = robots.find(r => {
          if (!r || !poi) return false;

          const distanceX = Math.abs((r.x || 0) - (poi.x || 0));
          const distanceY = Math.abs((r.y || 0) - (poi.y || 0));
          const distance = Math.sqrt(distanceX * distanceX + distanceY * distanceY);

          // Robot is at or heading to this charging station
          const isAtStation = distance < 15;
          const isGoingToStation = r.goal === poi.nodeId;
          const isIdleOrArrived = r.state === 'IDLE' || r.state === 'ARRIVED';
          const needsCharging = (r.batteryLevel || 100) < 95;

          // Consider station occupied if robot is there or heading there with low battery
          return (isAtStation && (isIdleOrArrived || needsCharging)) ||
            (isGoingToStation && needsCharging);
        });

        // Calculate estimated charging time based on battery level
        const timeRemaining = robotAtStation && robotAtStation.batteryLevel !== undefined
          ? Math.round((100 - robotAtStation.batteryLevel) * 0.5) // ~0.5s per 1% battery
          : undefined;

        return {
          ...poi,
          occupied: !!robotAtStation,
          robotId: robotAtStation?.id,
          timeRemaining
        };
      });
    } catch (error) {
      console.error('Error in getChargingStations:', error);
      return [];
    }
  },

  getTasksByRobot: (robotId: number) => {
    const tasks = Array.from(get().tasks.values());
    return tasks.filter(t => t.assignedRobotId === robotId);
  },

  getEfficiency: () => {
    try {
      const uptime = get().uptime || 0;
      const completed = get().completedTaskIds?.size || 0;
      if (!uptime || uptime === 0) return 0;
      const minutes = uptime / 60;
      return minutes > 0 ? completed / minutes : 0;
    } catch (error) {
      console.error('Error calculating efficiency:', error);
      return 0;
    }
  },

  getTotalTaskCount: () => {
    return get().tasks.size;
  }
}));

// Helper hook for system statistics
export const useSystemStats = () => {
  const systemStats = useFleetStore(state => state.systemStats);
  const isBackendConnected = useFleetStore(state => state.isBackendConnected);
  const lastRobotUpdate = useFleetStore(state => state.lastRobotUpdate);
  const lastTaskUpdate = useFleetStore(state => state.lastTaskUpdate);

  return {
    ...systemStats,
    isBackendConnected,
    lastRobotUpdate,
    lastTaskUpdate,
    dataAge: Date.now() - lastRobotUpdate
  };
};

// Helper hook for fleet overview
export const useFleetOverview = () => {
  return useFleetStore((state) => ({
    totalRobots: state?.robots?.size || 0,
    activeRobots: state?.getActiveRobotCount?.() || 0,
    idleRobots: state?.getIdleRobotCount?.() || 0,
    chargingRobots: state?.getChargingRobotCount?.() || 0,
    avgBattery: state?.getAverageBattery?.() || 100
  }));
};

// Helper hook for task statistics
export const useTaskStats = () => {
  return useFleetStore((state) => ({
    totalTasks: state?.tasks?.size || 0,
    pending: state?.getPendingTaskCount?.() || 0,
    assigned: state?.getAssignedTaskCount?.() || 0,
    inProgress: state?.getInProgressTaskCount?.() || 0,
    completed: state?.getCompletedTaskCount?.() || 0
  }));
};

export default useFleetStore;
