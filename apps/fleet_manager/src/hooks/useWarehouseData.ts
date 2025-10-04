import { useState, useEffect, useCallback } from 'react';

// Types for the fleet management system
export interface AMRRobot {
  id: string;
  name: string;
  status: 'Active' | 'Idle' | 'Charging' | 'Maintenance' | 'Error';
  battery: number;
  currentTask: string | null;
  location: { x: number; y: number; zone: string };
  tasksCompleted: number;
  efficiency: number;
  lastMaintenance: Date;
}

export interface WarehouseZone {
  id: string;
  name: string;
  type: 'Storage' | 'Picking' | 'Charging' | 'Maintenance';
  status: 'Active' | 'Inactive';
  capacity: number;
  packages: number;
  utilization: number;
  robotsAssigned: number;
}

export interface WarehouseAlert {
  id: string;
  type: 'error' | 'warning' | 'info' | 'success';
  title: string;
  message: string;
  timestamp: Date;
  acknowledged: boolean;
  robotId?: string;
  zoneId?: string;
}

export interface WarehouseAnalytics {
  totalRobots: number;
  activeRobots: number;
  avgBattery: number;
  totalTasks: number;
  completedTasks: number;
  efficiency: number;
  uptime: number;
  throughput: number;
}

// Custom hook for AMR fleet management
export const useAMRFleet = () => {
  const [robots, setRobots] = useState<AMRRobot[]>([]);

  useEffect(() => {
    // Initialize fleet with sample data
    const initialRobots: AMRRobot[] = [
      {
        id: 'AMR-001',
        name: 'Atlas',
        status: 'Active',
        battery: 85,
        currentTask: 'Transporting package to Zone A',
        location: { x: 12, y: 8, zone: 'Zone A' },
        tasksCompleted: 847,
        efficiency: 94.5,
        lastMaintenance: new Date(2024, 9, 1)
      },
      {
        id: 'AMR-002',
        name: 'Titan',
        status: 'Charging',
        battery: 23,
        currentTask: null,
        location: { x: 5, y: 15, zone: 'Charging Station' },
        tasksCompleted: 692,
        efficiency: 89.2,
        lastMaintenance: new Date(2024, 9, 15)
      },
      {
        id: 'AMR-003',
        name: 'Hermes',
        status: 'Active',
        battery: 67,
        currentTask: 'Picking from Zone C',
        location: { x: 20, y: 12, zone: 'Zone C' },
        tasksCompleted: 1034,
        efficiency: 96.8,
        lastMaintenance: new Date(2024, 8, 28)
      },
      {
        id: 'AMR-004',
        name: 'Apollo',
        status: 'Maintenance',
        battery: 45,
        currentTask: null,
        location: { x: 2, y: 3, zone: 'Maintenance Bay' },
        tasksCompleted: 523,
        efficiency: 87.3,
        lastMaintenance: new Date(2024, 10, 3)
      },
      {
        id: 'AMR-005',
        name: 'Artemis',
        status: 'Idle',
        battery: 92,
        currentTask: null,
        location: { x: 15, y: 6, zone: 'Zone B' },
        tasksCompleted: 758,
        efficiency: 91.7,
        lastMaintenance: new Date(2024, 9, 10)
      },
      {
        id: 'AMR-006',
        name: 'Poseidon',
        status: 'Error',
        battery: 78,
        currentTask: 'System diagnostics',
        location: { x: 18, y: 20, zone: 'Zone D' },
        tasksCompleted: 445,
        efficiency: 82.4,
        lastMaintenance: new Date(2024, 9, 5)
      }
    ];

    setRobots(initialRobots);

    // Simulate real-time updates
    const interval = setInterval(() => {
      setRobots(prevRobots => 
        prevRobots.map(robot => {
          const updates: Partial<AMRRobot> = {};

          // Battery updates
          if (robot.status === 'Charging') {
            updates.battery = Math.min(100, robot.battery + Math.random() * 3);
            if (updates.battery >= 95) {
              updates.status = 'Idle';
              updates.currentTask = null;
            }
          } else if (robot.status === 'Active') {
            updates.battery = Math.max(15, robot.battery - Math.random() * 0.5);
            if (updates.battery <= 20) {
              updates.status = 'Charging';
              updates.currentTask = null;
              updates.location = { x: 5, y: 15, zone: 'Charging Station' };
            }
          }

          // Task completion simulation
          if (robot.status === 'Active' && Math.random() < 0.1) {
            updates.tasksCompleted = robot.tasksCompleted + 1;
            // Efficiency slightly increases with completed tasks
            updates.efficiency = Math.min(100, robot.efficiency + Math.random() * 0.1);
          }

          // Random status changes for idle robots
          if (robot.status === 'Idle' && Math.random() < 0.05) {
            updates.status = 'Active';
            updates.currentTask = [
              'Transporting package to Zone A',
              'Picking from Zone B',
              'Delivering to Zone C',
              'Restocking Zone D'
            ][Math.floor(Math.random() * 4)];
          }

          return { ...robot, ...updates };
        })
      );
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  const sendRobotToCharge = useCallback((robotId: string) => {
    setRobots(prevRobots =>
      prevRobots.map(robot =>
        robot.id === robotId
          ? {
              ...robot,
              status: 'Charging',
              currentTask: null,
              location: { x: 5, y: 15, zone: 'Charging Station' }
            }
          : robot
      )
    );
  }, []);

  const sendRobotToMaintenance = useCallback((robotId: string) => {
    setRobots(prevRobots =>
      prevRobots.map(robot =>
        robot.id === robotId
          ? {
              ...robot,
              status: 'Maintenance',
              currentTask: null,
              location: { x: 2, y: 3, zone: 'Maintenance Bay' }
            }
          : robot
      )
    );
  }, []);

  const assignTask = useCallback((robotId: string, task: string) => {
    setRobots(prevRobots =>
      prevRobots.map(robot =>
        robot.id === robotId && robot.status === 'Idle'
          ? { ...robot, status: 'Active', currentTask: task }
          : robot
      )
    );
  }, []);

  return {
    robots,
    sendRobotToCharge,
    sendRobotToMaintenance,
    assignTask
  };
};

// Custom hook for warehouse zones
export const useWarehouseZones = () => {
  const [zones, setZones] = useState<WarehouseZone[]>([]);

  useEffect(() => {
    const initialZones: WarehouseZone[] = [
      {
        id: 'zone-a',
        name: 'Storage Zone A',
        type: 'Storage',
        status: 'Active',
        capacity: 1000,
        packages: 847,
        utilization: 84.7,
        robotsAssigned: 2
      },
      {
        id: 'zone-b',
        name: 'Picking Zone B',
        type: 'Picking',
        status: 'Active',
        capacity: 500,
        packages: 234,
        utilization: 46.8,
        robotsAssigned: 1
      },
      {
        id: 'zone-c',
        name: 'Storage Zone C',
        type: 'Storage',
        status: 'Active',
        capacity: 800,
        packages: 623,
        utilization: 77.9,
        robotsAssigned: 2
      },
      {
        id: 'zone-d',
        name: 'Distribution Zone',
        type: 'Picking',
        status: 'Active',
        capacity: 300,
        packages: 189,
        utilization: 63.0,
        robotsAssigned: 1
      },
      {
        id: 'charging',
        name: 'Charging Station',
        type: 'Charging',
        status: 'Active',
        capacity: 10,
        packages: 0,
        utilization: 30.0,
        robotsAssigned: 1
      },
      {
        id: 'maintenance',
        name: 'Maintenance Bay',
        type: 'Maintenance',
        status: 'Active',
        capacity: 5,
        packages: 0,
        utilization: 20.0,
        robotsAssigned: 1
      }
    ];

    setZones(initialZones);

    // Simulate zone updates
    const interval = setInterval(() => {
      setZones(prevZones =>
        prevZones.map(zone => ({
          ...zone,
          packages: zone.type === 'Storage' || zone.type === 'Picking'
            ? Math.max(0, zone.packages + Math.floor((Math.random() - 0.5) * 10))
            : zone.packages,
          utilization: zone.capacity > 0 
            ? Math.min(100, (zone.packages / zone.capacity) * 100)
            : zone.utilization
        }))
      );
    }, 10000);

    return () => clearInterval(interval);
  }, []);

  return { zones };
};

// Custom hook for warehouse analytics
export const useWarehouseAnalytics = () => {
  const { robots } = useAMRFleet();
  const [analytics, setAnalytics] = useState<WarehouseAnalytics>({
    totalRobots: 0,
    activeRobots: 0,
    avgBattery: 0,
    totalTasks: 0,
    completedTasks: 0,
    efficiency: 0,
    uptime: 0,
    throughput: 0
  });

  useEffect(() => {
    if (robots.length === 0) return;

    const activeRobots = robots.filter(r => r.status === 'Active').length;
    const avgBattery = robots.reduce((sum, r) => sum + r.battery, 0) / robots.length;
    const completedTasks = robots.reduce((sum, r) => sum + r.tasksCompleted, 0);
    const avgEfficiency = robots.reduce((sum, r) => sum + r.efficiency, 0) / robots.length;
    const operationalRobots = robots.filter(r => r.status !== 'Maintenance' && r.status !== 'Error').length;
    const uptime = (operationalRobots / robots.length) * 100;

    setAnalytics({
      totalRobots: robots.length,
      activeRobots,
      avgBattery: Math.round(avgBattery * 10) / 10,
      totalTasks: completedTasks + activeRobots * 10, // Estimate pending tasks
      completedTasks,
      efficiency: Math.round(avgEfficiency * 10) / 10,
      uptime: Math.round(uptime * 10) / 10,
      throughput: Math.round((completedTasks / 24) * 10) / 10 // Tasks per hour (simulated)
    });
  }, [robots]);

  return { analytics };
};

// Custom hook for warehouse alerts
export const useWarehouseAlerts = () => {
  const [alerts, setAlerts] = useState<WarehouseAlert[]>([]);
  const { robots } = useAMRFleet();

  useEffect(() => {
    // Generate alerts based on robot status
    const newAlerts: WarehouseAlert[] = [];

    robots.forEach(robot => {
      if (robot.battery < 25 && robot.status !== 'Charging') {
        newAlerts.push({
          id: `battery-${robot.id}`,
          type: 'warning',
          title: 'Low Battery Alert',
          message: `${robot.name} battery is at ${robot.battery}%`,
          timestamp: new Date(),
          acknowledged: false,
          robotId: robot.id
        });
      }

      if (robot.status === 'Error') {
        newAlerts.push({
          id: `error-${robot.id}`,
          type: 'error',
          title: 'Robot Error',
          message: `${robot.name} requires immediate attention`,
          timestamp: new Date(),
          acknowledged: false,
          robotId: robot.id
        });
      }

      const daysSinceLastMaintenance = Math.floor(
        (new Date().getTime() - robot.lastMaintenance.getTime()) / (1000 * 60 * 60 * 24)
      );

      if (daysSinceLastMaintenance > 30) {
        newAlerts.push({
          id: `maintenance-${robot.id}`,
          type: 'info',
          title: 'Maintenance Due',
          message: `${robot.name} is due for maintenance`,
          timestamp: new Date(),
          acknowledged: false,
          robotId: robot.id
        });
      }
    });

    // Add some general system alerts
    if (Math.random() > 0.95) {
      newAlerts.push({
        id: `system-${Date.now()}`,
        type: 'success',
        title: 'System Update',
        message: 'Fleet management system updated successfully',
        timestamp: new Date(),
        acknowledged: false
      });
    }

    setAlerts(prevAlerts => {
      const existingIds = new Set(prevAlerts.map(a => a.id));
      const filteredNewAlerts = newAlerts.filter(a => !existingIds.has(a.id));
      return [...prevAlerts, ...filteredNewAlerts].slice(-50); // Keep last 50 alerts
    });
  }, [robots]);

  const acknowledgeAlert = useCallback((alertId: string) => {
    setAlerts(prevAlerts =>
      prevAlerts.map(alert =>
        alert.id === alertId ? { ...alert, acknowledged: true } : alert
      )
    );
  }, []);

  const unacknowledgedCount = alerts.filter(a => !a.acknowledged).length;

  return {
    alerts: alerts.sort((a, b) => b.timestamp.getTime() - a.timestamp.getTime()),
    acknowledgeAlert,
    unacknowledgedCount
  };
};