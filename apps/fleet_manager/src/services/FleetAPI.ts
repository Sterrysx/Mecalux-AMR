// Fleet API Service - Polls backend JSON files at different frequencies
// robots.json: 1 Hz (C++ backend writes at this rate)
// tasks.json: 1 Hz (every 1000ms)
// map.json: 1 Hz (every 1000ms)

export interface RobotState {
  id: number;
  x: number;              // Pixel coordinates (0.1m resolution)
  y: number;
  vx: number;             // Velocity components
  vy: number;
  state: 'IDLE' | 'MOVING' | 'COMPUTING_PATH' | 'ARRIVED' | 'COLLISION_WAIT' | 'CARRYING';
  goal: number | null;    // Current NavMesh node ID
  itinerary: number;      // Remaining waypoints count
  batteryLevel?: number;  // Battery percentage (0-100)
}

export interface TasksInfo {
  active: number;
  completed: number;
  pending: number;
}

export interface ChargingStationInfo {
  id: number;
  x: number;
  y: number;
  status: 'AVAILABLE' | 'OCCUPIED';
  robot: number | null;
}

export interface Task {
  id: number;
  sourceNode: number;     // Pickup POI node
  destinationNode: number; // Dropoff POI node
  status: 'PENDING' | 'ASSIGNED' | 'IN_PROGRESS' | 'COMPLETED';
  assignedRobotId?: number;
}

export interface DynamicObstacle {
  x: number;
  y: number;
  type: 'BOX' | 'TEMPORARY_BLOCKAGE';
}

export interface POI {
  id: string;
  type: 'CHARGING' | 'PICKUP' | 'DROPOFF';
  x: number;
  y: number;
  nodeId: number;
}

export interface FleetData {
  robots: RobotState[];
  timestamp: number;
  tick?: number;
  tasks?: TasksInfo;
  charging_stations?: ChargingStationInfo[];
}

export interface TaskData {
  tasks: Task[];
  timestamp: number;
}

export interface MapData {
  obstacles: DynamicObstacle[];
  timestamp: number;
}

export interface SystemStats {
  currentScenario?: 'A' | 'B' | 'C';
  vrpSolverActive?: boolean;
  lastVRPSolveTime?: number;
  avgPathQueryTime?: number;
  physicsLoopHz?: number;
  throughput?: number;
}

class FleetAPIService {
  private baseURL: string;
  private robotsInterval: number | null = null;
  private tasksInterval: number | null = null;
  private mapInterval: number | null = null;
  private isPolling: boolean = false;

  // Callbacks for data updates
  private onRobotsUpdate?: (data: FleetData) => void;
  private onTasksUpdate?: (data: TaskData) => void;
  private onMapUpdate?: (data: MapData) => void;
  private onError?: (error: Error) => void;

  constructor(baseURL: string = 'http://localhost:3001') {
    this.baseURL = baseURL;
  }

  // Start polling all endpoints
  startPolling(callbacks: {
    onRobotsUpdate: (data: FleetData) => void;
    onTasksUpdate: (data: TaskData) => void;
    onMapUpdate: (data: MapData) => void;
    onError?: (error: Error) => void;
  }) {
    if (this.isPolling) {
      console.warn('FleetAPI: Already polling');
      return;
    }

    this.onRobotsUpdate = callbacks.onRobotsUpdate;
    this.onTasksUpdate = callbacks.onTasksUpdate;
    this.onMapUpdate = callbacks.onMapUpdate;
    this.onError = callbacks.onError;

    this.isPolling = true;

    // Poll robots at 1 Hz (matches C++ write rate)
    this.robotsInterval = window.setInterval(() => {
      this.fetchRobots();
    }, 1000);

    // Poll tasks at 1 Hz (every 1000ms)
    this.tasksInterval = window.setInterval(() => {
      this.fetchTasks();
    }, 1000);

    // Poll map at 1 Hz (every 1000ms)
    this.mapInterval = window.setInterval(() => {
      this.fetchMap();
    }, 1000);

    // Initial fetch
    this.fetchRobots();
    this.fetchTasks();
    this.fetchMap();

    console.log('FleetAPI: Started polling (robots: 1Hz, tasks: 1Hz, map: 1Hz)');
  }

  // Stop all polling
  stopPolling() {
    if (!this.isPolling) return;

    if (this.robotsInterval) clearInterval(this.robotsInterval);
    if (this.tasksInterval) clearInterval(this.tasksInterval);
    if (this.mapInterval) clearInterval(this.mapInterval);

    this.robotsInterval = null;
    this.tasksInterval = null;
    this.mapInterval = null;
    this.isPolling = false;

    console.log('FleetAPI: Stopped polling');
  }

  // Fetch robots data from the new C++ direct-write robots.json
  private async fetchRobots() {
    try {
      const response = await fetch(`${this.baseURL}/api/output/robots.json`, {
        cache: 'no-cache'
      });

      if (!response.ok) {
        throw new Error(`Failed to fetch robots: ${response.status}`);
      }

      const data: FleetData = await response.json();
      this.onRobotsUpdate?.(data);
    } catch (error) {
      // Only log error occasionally to avoid spam
      if (Math.random() < 0.05) { // 5% of the time
        console.warn('FleetAPI: Error fetching robots', error);
      }
      this.onError?.(error as Error);
    }
  }

  // Fetch tasks data
  private async fetchTasks() {
    try {
      const response = await fetch(`${this.baseURL}/api/output/tasks.json`, {
        cache: 'no-cache'
      });

      if (!response.ok) {
        throw new Error(`Failed to fetch tasks: ${response.status}`);
      }

      const data: TaskData = await response.json();
      this.onTasksUpdate?.(data);
    } catch (error) {
      console.warn('FleetAPI: Error fetching tasks', error);
      this.onError?.(error as Error);
    }
  }

  // Fetch map data
  private async fetchMap() {
    try {
      const response = await fetch(`${this.baseURL}/api/output/map.json`, {
        cache: 'no-cache'
      });

      if (!response.ok) {
        throw new Error(`Failed to fetch map: ${response.status}`);
      }

      const data: MapData = await response.json();
      this.onMapUpdate?.(data);
    } catch (error) {
      console.warn('FleetAPI: Error fetching map', error);
      this.onError?.(error as Error);
    }
  }

  // Fetch POI configuration (one-time)
  async fetchPOIs(): Promise<POI[]> {
    try {
      const response = await fetch(`${this.baseURL}/api/pois.json`);
      if (!response.ok) {
        throw new Error(`Failed to fetch POIs: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error('FleetAPI: Error fetching POIs', error);
      return [];
    }
  }

  // Inject new tasks
  async injectTasks(tasks: Array<{ pickup: string; dropoff: string }>) {
    try {
      const response = await fetch(`${this.baseURL}/api/fleet/inject-tasks`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tasks })
      });

      if (!response.ok) {
        throw new Error(`Failed to inject tasks: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error('FleetAPI: Error injecting tasks', error);
      throw error;
    }
  }

  // Send robot to charging station
  async sendRobotToCharge(robotId: number, chargingStationId: string) {
    try {
      const response = await fetch(`${this.baseURL}/api/fleet/robot/${robotId}/charge`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ chargingStationId })
      });

      if (!response.ok) {
        throw new Error(`Failed to send robot to charge: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error('FleetAPI: Error sending robot to charge', error);
      throw error;
    }
  }

  // Get system statistics
  async getSystemStats(): Promise<SystemStats> {
    try {
      const response = await fetch(`${this.baseURL}/api/fleet/stats`);
      if (!response.ok) {
        throw new Error(`Failed to fetch stats: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error('FleetAPI: Error fetching stats', error);
      return {};
    }
  }

  // Check if backend is running
  async healthCheck(): Promise<boolean> {
    try {
      const response = await fetch(`${this.baseURL}/health`, {
        method: 'GET',
        cache: 'no-cache'
      });
      return response.ok;
    } catch (error) {
      return false;
    }
  }
}

// Singleton instance
export const fleetAPI = new FleetAPIService();

export default fleetAPI;
