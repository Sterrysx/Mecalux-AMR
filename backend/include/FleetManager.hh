/**
 * @file FleetManager.hh
 * @brief System Orchestrator - Central multi-threaded engine
 * 
 * The FleetManager is the top-level controller that owns all data and
 * orchestrates the entire AMR system. It bridges:
 * - Layer 1: Maps, NavMesh, POIs (static infrastructure)
 * - Layer 2: VRP Solver, RobotAgents (logical/planning state)
 * - Layer 3: Pathfinding, RobotDrivers (physical/simulation state)
 * 
 * Threading Model:
 * - Main Thread (1 Hz): Strategic decisions - VRP solving, task assignment
 * - Fleet Thread (20 Hz): Physics simulation - position updates, collision avoidance
 * - Obstacle Thread (1 Hz): Map updates - dynamic obstacle painting
 */

#ifndef BACKEND_FLEETMANAGER_HH
#define BACKEND_FLEETMANAGER_HH

#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <future>
#include <queue>
#include <optional>

// Layer 1 includes
#include "StaticBitMap.hh"
#include "InflatedBitMap.hh"
#include "DynamicBitMap.hh"
#include "NavMesh.hh"
#include "NavMeshGenerator.hh"
#include "POIRegistry.hh"
#include "Resolution.hh"
#include "Coordinates.hh"

// Layer 2 includes
#include "RobotAgent.hh"
#include "Task.hh"
#include "TaskLoader.hh"
#include "CostMatrixProvider.hh"
#include "IVRPSolver.hh"
#include "TabuSearch.hh"
#include "SimulatedAnnealing.hh"
#include "HillClimbing.hh"

// Layer 3 includes
#include "Core/RobotDriver.hh"
#include "Core/FastLoopManager.hh"
#include "Pathfinding/PathfindingService.hh"
#include "Physics/ObstacleData.hh"

// API includes
#include "../api/APIService.hh"

namespace Backend {

/**
 * @brief System configuration loaded from JSON.
 */
struct SystemConfig {
    // Tick rates
    float orcaTickMs = 50.0f;           ///< Physics loop tick (20 Hz)
    float warehouseTickMs = 1000.0f;    ///< Strategic loop tick (1 Hz)
    float obstacleTickMs = 1000.0f;     ///< Obstacle loop tick (1 Hz)
    
    // Robot parameters
    float robotRadiusMeters = 0.3f;     ///< Robot collision radius
    float robotSpeedMps = 1.6f;         ///< Robot average speed (m/s)
    
    // Resolution
    Common::Resolution mapResolution = Common::Resolution::DECIMETERS;
    
    // Paths
    std::string mapPath = "layer1/assets/map_layout.txt";
    std::string poiConfigPath = "layer1/assets/poi_config.json";
    std::string taskPath = "../api/set_of_tasks.json";
    
    // Fleet size (0 = auto from charging stations)
    int numRobots = 0;
    
    // Simulation mode
    bool batchMode = false;             ///< Batch mode: no sleep, auto-terminate
    
    // Dynamic Scheduling Configuration
    int batchThreshold = 5;              ///< If > threshold tasks arrive, trigger full re-plan
    int estimatedReplanTimeMs = 100;     ///< Estimated VRP solver time in ms
    
    /**
     * @brief Load configuration from JSON file.
     */
    static SystemConfig LoadFromJSON(const std::string& filepath);
};

/**
 * @brief Fleet statistics for monitoring.
 */
struct FleetStats {
    int totalTasks = 0;
    int completedTasks = 0;
    int pendingTasks = 0;
    double simulationTime = 0.0;
    int fleetLoopCount = 0;
    int mainLoopCount = 0;
    int obstacleLoopCount = 0;
    
    void Print() const;
};

/**
 * @brief Central orchestrator for the AMR system.
 * 
 * The FleetManager owns:
 * - Layer 1 data: Maps, NavMesh, POIs
 * - Layer 2 data: RobotAgents (logical state), Tasks, VRP Solver
 * - Layer 3 data: RobotDrivers (physical state), PathfindingService
 * 
 * Threading:
 * - runMainLoop(): 1 Hz - Check for idle robots, run VRP solver
 * - runFleetLoop(): 20 Hz - Update physics, sync positions
 * - runObstacleLoop(): 1 Hz - Update dynamic obstacles
 */
class FleetManager {
private:
    // =========================================================================
    // CONFIGURATION
    // =========================================================================
    
    SystemConfig config_;
    std::string basePath_;  ///< Base path for relative file paths
    
    // =========================================================================
    // LAYER 1: Infrastructure
    // =========================================================================
    
    std::unique_ptr<Layer1::StaticBitMap> staticMap_;
    std::unique_ptr<Layer1::InflatedBitMap> inflatedMap_;
    std::unique_ptr<Layer1::DynamicBitMap> dynamicMap_;
    std::unique_ptr<Layer1::NavMesh> navMesh_;
    std::unique_ptr<Layer1::POIRegistry> poiRegistry_;
    
    // =========================================================================
    // LAYER 2: Planning
    // =========================================================================
    
    /// The Logical Agents (Planner view) - Robot ID → Agent
    std::map<int, Layer2::RobotAgent> fleetRegistry_;
    
    /// VRP Solver (Strategy Pattern)
    std::unique_ptr<Layer2::IVRPSolver> vrpSolver_;
    
    /// Cost matrix for path planning
    std::unique_ptr<Layer2::CostMatrixProvider> costMatrix_;
    
    /// Pending tasks (from JSON or dynamically added)
    std::vector<Layer2::Task> pendingTasks_;
    
    /// Injection queue for new tasks arriving dynamically
    std::queue<Layer2::Task> injectionQueue_;
    std::mutex injectionMutex_;          ///< Protects injectionQueue_
    
    // =========================================================================
    // LAYER 3: Physics
    // =========================================================================
    
    /// The Physical Drivers (Physics view)
    std::vector<std::unique_ptr<Layer3::Core::RobotDriver>> drivers_;
    
    // =========================================================================
    // THREADING
    // =========================================================================
    
    std::thread mainThread_;        ///< Strategic loop (1 Hz)
    std::thread fleetThread_;       ///< Physics loop (20 Hz)
    std::thread obstacleThread_;    ///< Map update loop (1 Hz)
    
    std::atomic<bool> running_;     ///< Control flag for threads
    
    std::mutex fleetMutex_;         ///< Protects fleetRegistry_ and drivers_
    std::mutex mapMutex_;           ///< Protects dynamicMap_
    std::mutex taskMutex_;          ///< Protects pendingTasks_
    
    // =========================================================================
    // API SERVICE
    // =========================================================================
    
    API::APIService apiService_;    ///< File-based API for visualization
    
    // =========================================================================
    // STATISTICS
    // =========================================================================
    
    FleetStats stats_;
    std::chrono::steady_clock::time_point startTime_;
    
    /// Track total waypoints visited (every 2 = 1 task completed)
    std::atomic<int> totalWaypointsVisited_{0};
    
    // =========================================================================
    // DYNAMIC SCHEDULING STATE (Scenarios A, B, C)
    // =========================================================================
    
    /// Background re-planning future (Scenario C)
    std::future<Layer2::VRPResult> replanFuture_;
    std::atomic<bool> replanInProgress_{false};
    
    /// Robots waiting for re-plan to complete (Scenario C)
    std::vector<int> robotsWaitingForReplan_;
    
    /// Task counter for unique IDs
    std::atomic<int> nextTaskId_{1000};
    
    /// Track if initial boot VRP has run (Scenario A)
    bool initialSolveComplete_{false};
    
    // =========================================================================
    // CALLBACKS
    // =========================================================================
    
    using TaskCompletedCallback = std::function<void(int robotId, int taskId)>;
    using RobotArrivedCallback = std::function<void(int robotId, int nodeId)>;
    
    TaskCompletedCallback onTaskCompleted_;
    RobotArrivedCallback onRobotArrived_;

public:
    // =========================================================================
    // CONSTRUCTOR & DESTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct FleetManager with configuration.
     * 
     * @param config System configuration
     * @param basePath Base path for relative file paths (defaults to current dir)
     */
    explicit FleetManager(const SystemConfig& config = SystemConfig(), 
                          const std::string& basePath = ".");
    
    /**
     * @brief Destructor - ensures threads are stopped.
     */
    ~FleetManager();
    
    // =========================================================================
    // LIFECYCLE
    // =========================================================================
    
    /**
     * @brief Initialize the system (load map, build NavMesh, etc.).
     * 
     * @return true if initialization succeeded
     */
    bool Initialize();
    
    /**
     * @brief Start all worker threads.
     */
    void Start();
    
    /**
     * @brief Stop all worker threads and clean up.
     */
    void Stop();
    
    /**
     * @brief Check if the system is running.
     */
    bool IsRunning() const { return running_.load(); }
    
    /**
     * @brief Check if all tasks are complete (for batch mode auto-termination).
     * 
     * Returns true when:
     * - All pending tasks have been assigned
     * - All robot itineraries are empty
     * - All robots are IDLE
     */
    bool IsAllTasksComplete() const;
    
    // =========================================================================
    // TASK MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Load tasks from JSON file.
     * 
     * @param filepath Path to tasks JSON file
     * @return Number of tasks loaded
     */
    int LoadTasks(const std::string& filepath);
    
    /**
     * @brief Add a single task to the pending queue.
     * 
     * @param task Task to add
     */
    void AddTask(const Layer2::Task& task);
    
    /**
     * @brief Get number of pending tasks.
     */
    int GetPendingTaskCount() const;
    
    /**
     * @brief Inject new tasks dynamically (thread-safe).
     * 
     * This is the main entry point for Scenarios B and C.
     * Tasks are queued and processed on the next MainLoop tick.
     * 
     * @param tasks Vector of tasks to inject
     */
    void InjectTasks(const std::vector<Layer2::Task>& tasks);
    
    /**
     * @brief Inject a single task dynamically (thread-safe).
     * 
     * @param sourceNodeId Pickup node ID
     * @param destNodeId Dropoff node ID
     */
    void InjectTask(int sourceNodeId, int destNodeId);
    
    /**
     * @brief Get number of tasks in injection queue.
     */
    int GetInjectionQueueSize() const;
    
    /**
     * @brief Check if a background re-plan is in progress.
     */
    bool IsReplanInProgress() const { return replanInProgress_.load(); }
    
    // =========================================================================
    // MONITORING
    // =========================================================================
    
    /**
     * @brief Get current fleet statistics.
     */
    FleetStats GetStats() const;
    
    /**
     * @brief Print current robot states.
     */
    void PrintRobotStates() const;
    
    /**
     * @brief Get robot agent by ID.
     */
    const Layer2::RobotAgent* GetRobotAgent(int robotId) const;
    
    /**
     * @brief Get robot driver by ID.
     */
    const Layer3::Core::RobotDriver* GetRobotDriver(int robotId) const;
    
    // =========================================================================
    // CALLBACKS
    // =========================================================================
    
    void SetOnTaskCompleted(TaskCompletedCallback callback) {
        onTaskCompleted_ = std::move(callback);
    }
    
    void SetOnRobotArrived(RobotArrivedCallback callback) {
        onRobotArrived_ = std::move(callback);
    }

private:
    // =========================================================================
    // THREAD ENTRY POINTS
    // =========================================================================
    
    /**
     * @brief Strategic loop (1 Hz).
     * 
     * Responsibilities:
     * - Check for robots with empty itineraries
     * - Run VRP solver when tasks are pending
     * - Assign itineraries to robots
     */
    void runMainLoop();
    
    /**
     * @brief Physics loop (20 Hz).
     * 
     * Responsibilities:
     * - Update RobotDrivers (call UpdateLoop)
     * - Sync Layer 3 positions to Layer 2 agents
     * - Feed next goal from L2 itinerary to L3 driver
     */
    void runFleetLoop();
    
    /**
     * @brief Map update loop (1 Hz).
     * 
     * Responsibilities:
     * - Update dynamic obstacles on the map
     * - (Placeholder for now - can add forklift detection, etc.)
     */
    void runObstacleLoop();
    
    // =========================================================================
    // BRIDGE LOGIC
    // =========================================================================
    
    /**
     * @brief Sync Layer 3 driver position to Layer 2 agent.
     * 
     * Called from fleetLoop to update RobotAgent::currentNodeId
     * based on the driver's current position.
     * 
     * @param robotId Robot to sync
     */
    void syncL3toL2(int robotId);
    
    /**
     * @brief Feed next goal from Layer 2 itinerary to Layer 3 driver.
     * 
     * If L2 agent has an itinerary but L3 driver is idle,
     * pop the next node from the itinerary and call driver->SetGoal().
     * 
     * @param robotId Robot to update
     */
    void feedL2toL3(int robotId);
    
    /**
     * @brief Run VRP solver and assign itineraries to robots.
     * Scenario A: Full batch solve at startup or for large batches.
     */
    void runVRPSolver();
    
    /**
     * @brief Process injected tasks (called from MainLoop).
     * Decides between Scenario B (cheap insertion) or C (background re-plan).
     */
    void processInjectedTasks();
    
    /**
     * @brief Scenario B: Cheap insertion heuristic for small batches.
     * Inserts each task at the cheapest position in existing itineraries.
     * 
     * @param tasks Tasks to insert
     */
    void runCheapInsertion(const std::vector<Layer2::Task>& tasks);
    
    /**
     * @brief Scenario C: Launch background re-planning.
     * Starts VRP solver in a separate thread.
     * 
     * @param tasks Tasks to include in re-plan
     */
    void launchBackgroundReplan(const std::vector<Layer2::Task>& tasks);
    
    /**
     * @brief Check and apply background re-plan results.
     * Called from MainLoop to check if async solver finished.
     */
    void checkBackgroundReplan();
    
    /**
     * @brief Calculate insertion cost for a task into a robot's itinerary.
     * 
     * @param robot Robot to consider
     * @param task Task to insert
     * @return Additional cost (distance) to add this task
     */
    float calculateInsertionCost(const Layer2::RobotAgent& robot, const Layer2::Task& task) const;
    
    /**
     * @brief Estimate remaining time for a robot to complete current work.
     * Used in Scenario C to decide if robot should wait or proceed.
     * 
     * @param robotId Robot to check
     * @return Estimated remaining time in milliseconds
     */
    int estimateRobotRemainingTimeMs(int robotId) const;
    
    /**
     * @brief Find nearest NavMesh node ID for a given position.
     */
    int findNearestNode(const Common::Coordinates& pos) const;
    
    // =========================================================================
    // INITIALIZATION HELPERS
    // =========================================================================
    
    bool initializeLayer1();
    bool initializeLayer2();
    bool initializeLayer3();
    void createRobots();
};

} // namespace Backend

#endif // BACKEND_FLEETMANAGER_HH
