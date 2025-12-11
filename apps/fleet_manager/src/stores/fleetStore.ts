// Zustand Store for Fleet Management State
import { create } from 'zustand';
import { RobotState, Task, DynamicObstacle, POI, SystemStats, FleetData, TaskData, MapData } from '../services/FleetAPI';

interface FleetState {
  // Robot data
  robots: Map<number, RobotState>;
  lastRobotUpdate: number;
  
  // Task data
  tasks: Map<number, Task>;
  lastTaskUpdate: number;
  
  // Map data
  dynamicObstacles: DynamicObstacle[];
  pois: POI[];
  lastMapUpdate: number;
  
  // System state
  systemStats: SystemStats;
  isBackendConnected: boolean;
  errorCount: number;
  
  // Actions
  updateRobots: (data: FleetData) => void;
  updateTasks: (data: TaskData) => void;
  updateMap: (data: MapData) => void;
  setPOIs: (pois: POI[]) => void;
  updateSystemStats: (stats: SystemStats) => void;
  setBackendConnected: (connected: boolean) => void;
  incrementErrorCount: () => void;
  resetErrorCount: () => void;
  
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
}

export const useFleetStore = create<FleetState>((set, get) => ({
  // Initial state
  robots: new Map(),
  lastRobotUpdate: 0,
  tasks: new Map(),
  lastTaskUpdate: 0,
  dynamicObstacles: [],
  pois: [],
  lastMapUpdate: 0,
  systemStats: {},
  isBackendConnected: false,
  errorCount: 0,

  // Actions
  updateRobots: (data: FleetData) => {
    const robotsMap = new Map<number, RobotState>();
    data.robots.forEach(robot => {
      robotsMap.set(robot.id, robot);
    });
    
    set({
      robots: robotsMap,
      lastRobotUpdate: data.timestamp || Date.now()
    });
  },

  updateTasks: (data: TaskData) => {
    const tasksMap = new Map<number, Task>();
    data.tasks.forEach(task => {
      tasksMap.set(task.id, task);
    });
    
    set({
      tasks: tasksMap,
      lastTaskUpdate: data.timestamp || Date.now()
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

  // Computed getters
  getActiveRobotCount: () => {
    const robots = Array.from(get().robots.values());
    return robots.filter(r => r.state === 'MOVING' || r.state === 'CARRYING').length;
  },

  getIdleRobotCount: () => {
    const robots = Array.from(get().robots.values());
    return robots.filter(r => r.state === 'IDLE').length;
  },

  getChargingRobotCount: () => {
    const robots = Array.from(get().robots.values());
    // Assuming robots at charging stations have specific goal nodes
    return robots.filter(r => r.state === 'IDLE' && r.goal === null).length;
  },

  getAverageBattery: () => {
    const robots = Array.from(get().robots.values());
    if (robots.length === 0) return 0;
    
    const totalBattery = robots.reduce((sum, r) => sum + (r.batteryLevel || 100), 0);
    return Math.round(totalBattery / robots.length);
  },

  getPendingTaskCount: () => {
    return get().getTasksByStatus('PENDING').length;
  },

  getAssignedTaskCount: () => {
    return get().getTasksByStatus('ASSIGNED').length;
  },

  getInProgressTaskCount: () => {
    return get().getTasksByStatus('IN_PROGRESS').length;
  },

  getCompletedTaskCount: () => {
    return get().getTasksByStatus('COMPLETED').length;
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
  const totalRobots = useFleetStore(state => state.robots.size);
  const activeRobots = useFleetStore(state => state.getActiveRobotCount());
  const idleRobots = useFleetStore(state => state.getIdleRobotCount());
  const chargingRobots = useFleetStore(state => state.getChargingRobotCount());
  const avgBattery = useFleetStore(state => state.getAverageBattery());
  
  return {
    totalRobots,
    activeRobots,
    idleRobots,
    chargingRobots,
    avgBattery
  };
};

// Helper hook for task statistics
export const useTaskStats = () => {
  const totalTasks = useFleetStore(state => state.tasks.size);
  const pending = useFleetStore(state => state.getPendingTaskCount());
  const assigned = useFleetStore(state => state.getAssignedTaskCount());
  const inProgress = useFleetStore(state => state.getInProgressTaskCount());
  const completed = useFleetStore(state => state.getCompletedTaskCount());
  
  return {
    totalTasks,
    pending,
    assigned,
    inProgress,
    completed
  };
};

export default useFleetStore;
