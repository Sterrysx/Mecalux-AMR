/**
 * @file FleetManager.cc
 * @brief Implementation of the System Orchestrator
 * 
 * The FleetManager bridges all three layers:
 * - Layer 1: Static/Dynamic maps, NavMesh, POIs
 * - Layer 2: VRP Solver, RobotAgents (logical state)
 * - Layer 3: Pathfinding, RobotDrivers (physical state)
 */

#include "FleetManager.hh"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace Backend {

// =============================================================================
// SYSTEM CONFIG
// =============================================================================

SystemConfig SystemConfig::LoadFromJSON(const std::string& filepath) {
    SystemConfig config;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[FleetManager] Warning: Could not open config file: " 
                  << filepath << ", using defaults\n";
        return config;
    }
    
    // Simple JSON parsing (no external library)
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Parse key values (basic parsing)
    auto parseFloat = [&content](const std::string& key, float defaultVal) -> float {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        return std::stof(content.substr(pos + 1));
    };
    
    auto parseString = [&content](const std::string& key, const std::string& defaultVal) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        size_t start = content.find("\"", pos);
        if (start == std::string::npos) return defaultVal;
        size_t end = content.find("\"", start + 1);
        if (end == std::string::npos) return defaultVal;
        return content.substr(start + 1, end - start - 1);
    };
    
    config.orcaTickMs = parseFloat("orca_tick_ms", 50.0f);
    config.robotRadiusMeters = parseFloat("robot_radius_meters", 0.3f);
    config.poiConfigPath = parseString("poi_config_path", "layer1/assets/poi_config.json");
    
    std::cout << "[FleetManager] Loaded config from: " << filepath << "\n";
    
    return config;
}

// =============================================================================
// FLEET STATS
// =============================================================================

void FleetStats::Print() const {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     FLEET STATISTICS                          ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Total Tasks:     " << std::setw(10) << totalTasks << "                              ║\n";
    std::cout << "║ Completed:       " << std::setw(10) << completedTasks << "                              ║\n";
    std::cout << "║ Pending:         " << std::setw(10) << pendingTasks << "                              ║\n";
    std::cout << "║ Simulation Time: " << std::setw(10) << std::fixed << std::setprecision(2) 
              << simulationTime << " s                          ║\n";
    std::cout << "║ Fleet Loops:     " << std::setw(10) << fleetLoopCount << "                              ║\n";
    std::cout << "║ Main Loops:      " << std::setw(10) << mainLoopCount << "                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
}

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================

FleetManager::FleetManager(const SystemConfig& config, const std::string& basePath)
    : config_(config)
    , basePath_(basePath)
    , running_(false)
    , apiService_(basePath + "/../api")  // API at repo root: backend/../api
{
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FLEET MANAGER - SYSTEM ORCHESTRATOR                    ║\n";
    std::cout << "║                  Bridging Layers 1, 2, and 3 together                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
}

FleetManager::~FleetManager() {
    Stop();
}

// =============================================================================
// LIFECYCLE
// =============================================================================

bool FleetManager::Initialize() {
    std::cout << "\n[FleetManager] Initializing system...\n";
    
    // Load system configuration - merge with existing config (don't overwrite CLI args)
    std::string configPath = basePath_ + "/system_config.json";
    SystemConfig fileConfig = SystemConfig::LoadFromJSON(configPath);
    
    // Only override defaults from JSON, preserve CLI arguments
    config_.orcaTickMs = fileConfig.orcaTickMs;
    config_.robotRadiusMeters = fileConfig.robotRadiusMeters;
    config_.poiConfigPath = fileConfig.poiConfigPath;
    // Keep: numRobots, batchMode, robotSpeedMps, warehouseTickMs from CLI/defaults
    
    // Initialize all layers
    if (!initializeLayer1()) {
        std::cerr << "[FleetManager] ERROR: Layer 1 initialization failed!\n";
        return false;
    }
    
    if (!initializeLayer2()) {
        std::cerr << "[FleetManager] ERROR: Layer 2 initialization failed!\n";
        return false;
    }
    
    if (!initializeLayer3()) {
        std::cerr << "[FleetManager] ERROR: Layer 3 initialization failed!\n";
        return false;
    }
    
    // Create robots
    createRobots();
    
    std::cout << "[FleetManager] System initialized successfully!\n";
    return true;
}

void FleetManager::Start() {
    if (running_.load()) {
        std::cout << "[FleetManager] Already running!\n";
        return;
    }
    
    std::cout << "\n[FleetManager] Starting worker threads...\n";
    
    running_ = true;
    startTime_ = std::chrono::steady_clock::now();
    
    // Start threads
    mainThread_ = std::thread(&FleetManager::runMainLoop, this);
    fleetThread_ = std::thread(&FleetManager::runFleetLoop, this);
    obstacleThread_ = std::thread(&FleetManager::runObstacleLoop, this);
    
    std::cout << "[FleetManager] All threads started:\n";
    std::cout << "  - Main Thread (Strategic Loop): 1 Hz\n";
    std::cout << "  - Fleet Thread (Physics Loop): 20 Hz\n";
    std::cout << "  - Obstacle Thread (Map Loop): 1 Hz\n";
}

void FleetManager::Stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "\n[FleetManager] Stopping worker threads...\n";
    
    running_ = false;
    
    // Join threads
    if (mainThread_.joinable()) {
        mainThread_.join();
        std::cout << "  - Main Thread stopped\n";
    }
    if (fleetThread_.joinable()) {
        fleetThread_.join();
        std::cout << "  - Fleet Thread stopped\n";
    }
    if (obstacleThread_.joinable()) {
        obstacleThread_.join();
        std::cout << "  - Obstacle Thread stopped\n";
    }
    
    // Cleanup Layer 3 service
    Layer3::Pathfinding::PathfindingService::DestroyInstance();
    
    std::cout << "[FleetManager] All threads stopped.\n";
    
    // Print final stats
    stats_.Print();
}

// =============================================================================
// INITIALIZATION HELPERS
// =============================================================================

bool FleetManager::initializeLayer1() {
    std::cout << "\n[FleetManager] ═══════════════ Layer 1: Infrastructure ═══════════════\n";
    
    try {
        // Load static map
        std::string mapPath = basePath_ + "/" + config_.mapPath;
        std::cout << "[Layer 1] Loading map from: " << mapPath << "\n";
        
        staticMap_ = std::make_unique<Layer1::StaticBitMap>(
            Layer1::StaticBitMap::CreateFromFile(mapPath, config_.mapResolution)
        );
        
        auto [width, height] = staticMap_->GetDimensions();
        std::cout << "[Layer 1] Map size: " << width << "x" << height << " pixels\n";
        
        // Create inflated map
        std::cout << "[Layer 1] Creating inflated map (robot radius: " 
                  << config_.robotRadiusMeters << "m)...\n";
        inflatedMap_ = std::make_unique<Layer1::InflatedBitMap>(
            *staticMap_, config_.robotRadiusMeters
        );
        
        // Create dynamic map (copy of inflated)
        dynamicMap_ = std::make_unique<Layer1::DynamicBitMap>(*staticMap_);
        
        // Generate NavMesh
        std::cout << "[Layer 1] Generating NavMesh...\n";
        navMesh_ = std::make_unique<Layer1::NavMesh>();
        Layer1::NavMeshGenerator generator;
        generator.ComputeRecast(*inflatedMap_, *navMesh_);
        
        std::cout << "[Layer 1] NavMesh: " << navMesh_->GetAllNodes().size() << " nodes\n";
        
        // Load POI Registry
        std::string poiPath = basePath_ + "/" + config_.poiConfigPath;
        std::cout << "[Layer 1] Loading POIs from: " << poiPath << "\n";
        
        poiRegistry_ = std::make_unique<Layer1::POIRegistry>();
        if (poiRegistry_->LoadFromJSON(poiPath)) {
            poiRegistry_->MapToNavMesh(*navMesh_);
            std::cout << "[Layer 1] POIs loaded: " << poiRegistry_->GetPOICount() << " total\n";
            std::cout << "          - Charging: " << poiRegistry_->GetNodesByType(Layer1::POIType::CHARGING).size() << "\n";
            std::cout << "          - Pickup:   " << poiRegistry_->GetNodesByType(Layer1::POIType::PICKUP).size() << "\n";
            std::cout << "          - Dropoff:  " << poiRegistry_->GetNodesByType(Layer1::POIType::DROPOFF).size() << "\n";
        } else {
            std::cerr << "[Layer 1] Warning: Could not load POI config\n";
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Layer 1] Exception: " << e.what() << "\n";
        return false;
    }
}

bool FleetManager::initializeLayer2() {
    std::cout << "\n[FleetManager] ═══════════════ Layer 2: Planning ═══════════════\n";
    
    try {
        // Create cost matrix provider
        std::cout << "[Layer 2] Creating cost matrix provider...\n";
        costMatrix_ = std::make_unique<Layer2::CostMatrixProvider>(*navMesh_);
        
        // Precompute costs for POI nodes
        if (poiRegistry_) {
            std::vector<int> poiNodes;
            
            // Gather all POI nodes
            for (int node : poiRegistry_->GetNodesByType(Layer1::POIType::CHARGING)) {
                poiNodes.push_back(node);
            }
            for (int node : poiRegistry_->GetNodesByType(Layer1::POIType::PICKUP)) {
                poiNodes.push_back(node);
            }
            for (int node : poiRegistry_->GetNodesByType(Layer1::POIType::DROPOFF)) {
                poiNodes.push_back(node);
            }
            
            if (!poiNodes.empty()) {
                std::cout << "[Layer 2] Precomputing costs for " << poiNodes.size() << " POI nodes...\n";
                int computed = costMatrix_->PrecomputeForNodes(poiNodes);
                std::cout << "[Layer 2] Computed " << computed << " node pairs\n";
            }
        }
        
        // Create VRP solver (ALNS - Adaptive Large Neighborhood Search)
        // ALNS uses "Destroy and Repair" with Regret-2 insertion
        // Parameters: iterations=100, destruction=25%, seed=42
        std::cout << "[Layer 2] Creating VRP solver (ALNS)...\n";
        vrpSolver_ = std::make_unique<Layer2::ALNS>(100, 0.25, 42);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Layer 2] Exception: " << e.what() << "\n";
        return false;
    }
}

bool FleetManager::initializeLayer3() {
    std::cout << "\n[FleetManager] ═══════════════ Layer 3: Physics ═══════════════\n";
    
    try {
        // Initialize pathfinding service
        std::cout << "[Layer 3] Initializing PathfindingService...\n";
        auto& pathService = Layer3::Pathfinding::PathfindingService::GetInstance();
        pathService.Initialize(*inflatedMap_);
        
        std::cout << "[Layer 3] PathfindingService ready\n";
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Layer 3] Exception: " << e.what() << "\n";
        return false;
    }
}

void FleetManager::createRobots() {
    std::cout << "\n[FleetManager] ═══════════════ Creating Robots ═══════════════\n";
    
    // Get charging stations - these define the fleet size
    std::vector<int> chargingNodes;
    if (poiRegistry_) {
        chargingNodes = poiRegistry_->GetNodesByType(Layer1::POIType::CHARGING);
    }
    
    // Dynamic fleet sizing: If numRobots == 0 (auto), use charging station count
    int numRobots = config_.numRobots;
    if (numRobots <= 0) {
        numRobots = static_cast<int>(chargingNodes.size());
        std::cout << "[FleetManager] Auto-sizing fleet to " << numRobots 
                  << " robots (matching charging stations)\n";
    }
    
    // If no charging stations at all, fall back to first NavMesh nodes
    if (chargingNodes.empty()) {
        std::cerr << "[FleetManager] WARNING: No charging stations found! Using NavMesh nodes.\n";
        const auto& nodes = navMesh_->GetAllNodes();
        for (int i = 0; i < numRobots && i < static_cast<int>(nodes.size()); ++i) {
            chargingNodes.push_back(i);
        }
    }
    
    // Ensure we don't create more robots than charging stations
    if (numRobots > static_cast<int>(chargingNodes.size())) {
        std::cerr << "[FleetManager] WARNING: Reducing robots from " << numRobots 
                  << " to " << chargingNodes.size() << " (matching charging stations)\n";
        numRobots = static_cast<int>(chargingNodes.size());
    }
    
    std::cout << "[FleetManager] Creating " << numRobots << " robots...\n";
    
    for (int i = 0; i < numRobots; ++i) {
        // Each robot gets its own unique charging station
        int startNode = chargingNodes[i];
        Common::Coordinates startPos = navMesh_->GetAllNodes()[startNode].coords;
        
        // Create Layer 2 agent (logical state)
        Layer2::RobotAgent agent(
            i,                              // robot ID
            1.0f,                           // battery capacity
            chargingNodes[i],               // charging station (unique per robot)
            config_.robotSpeedMps,          // speed
            1                               // capacity (packets)
        );
        agent.SetCurrentNodeId(startNode);
        agent.SetStatus(Layer2::RobotStatus::IDLE);
        
        {
            std::lock_guard<std::mutex> lock(fleetMutex_);
            fleetRegistry_[i] = agent;
        }
        
        // Create Layer 3 driver (physical state)
        auto driver = std::make_unique<Layer3::Core::RobotDriver>(
            i,
            startPos,
            *navMesh_
        );
        
        // Set the current node ID so API reports valid targetNodeId from start
        driver->SetCurrentNodeId(startNode);
        
        // Configure driver with robot parameters
        Layer3::Core::DriverConfig driverConfig;
        driverConfig.maxSpeed = config_.robotSpeedMps * 10.0;  // m/s to decimeters/s
        driverConfig.robotRadius = config_.robotRadiusMeters * 10.0;  // m to decimeters
        driverConfig.waypointThreshold = 5.0;
        driverConfig.goalThreshold = 3.0;
        driver->SetConfig(driverConfig);
        
        // Set goal reached callback
        driver->SetOnGoalReached([this](int robotId, int goalNode) {
            std::cout << "[FleetManager] Robot " << robotId << " reached node " << goalNode << "\n";
            
            // Track waypoints for task completion (2 waypoints = 1 task)
            totalWaypointsVisited_.fetch_add(1);
            
            // Note: onRobotArrived_ callback skipped due to thread safety concerns
            // The callback is set from main thread but called from fleet thread
        });
        
        {
            std::lock_guard<std::mutex> lock(fleetMutex_);
            drivers_.push_back(std::move(driver));
        }
        
        std::cout << "  Robot " << i << ": Start node=" << startNode 
                  << ", Position=(" << startPos.x << "," << startPos.y << ")\n";
    }
    
    std::cout << "[FleetManager] " << numRobots << " robots created\n";
}

// =============================================================================
// TASK MANAGEMENT
// =============================================================================

int FleetManager::LoadTasks(const std::string& filepath) {
    std::cout << "\n[FleetManager] Loading tasks from: " << filepath << "\n";
    
    if (!poiRegistry_) {
        std::cerr << "[FleetManager] ERROR: POIRegistry not initialized!\n";
        return 0;
    }
    
    std::vector<Layer2::Task> tasks = Layer2::TaskLoader::LoadTasksWithPOI(
        filepath, *navMesh_, *poiRegistry_
    );
    
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        pendingTasks_ = tasks;
        stats_.totalTasks = static_cast<int>(tasks.size());
        stats_.pendingTasks = static_cast<int>(tasks.size());
    }
    
    std::cout << "[FleetManager] Loaded " << tasks.size() << " tasks\n";
    return static_cast<int>(tasks.size());
}

void FleetManager::AddTask(const Layer2::Task& task) {
    std::lock_guard<std::mutex> lock(taskMutex_);
    pendingTasks_.push_back(task);
    stats_.totalTasks++;
    stats_.pendingTasks++;
}

int FleetManager::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(taskMutex_));
    return static_cast<int>(pendingTasks_.size());
}

void FleetManager::InjectTasks(const std::vector<Layer2::Task>& tasks) {
    if (tasks.empty()) return;
    
    std::lock_guard<std::mutex> lock(injectionMutex_);
    for (const auto& task : tasks) {
        injectionQueue_.push(task);
    }
    
    std::cout << "[FleetManager] Injected " << tasks.size() << " new tasks (queue size: " 
              << injectionQueue_.size() << ")\n";
}

void FleetManager::InjectTask(int sourceNodeId, int destNodeId) {
    Layer2::Task task;
    task.taskId = nextTaskId_.fetch_add(1);
    task.sourceNode = sourceNodeId;
    task.destinationNode = destNodeId;
    
    std::lock_guard<std::mutex> lock(injectionMutex_);
    injectionQueue_.push(task);
    
    std::cout << "[FleetManager] Injected task " << task.taskId 
              << " (" << sourceNodeId << " → " << destNodeId << ")\n";
}

int FleetManager::GetInjectionQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(injectionMutex_));
    return static_cast<int>(injectionQueue_.size());
}

bool FleetManager::IsAllTasksComplete() const {
    // Check injection queue
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(injectionMutex_));
        if (!injectionQueue_.empty()) {
            return false;
        }
    }
    
    // Check pending tasks
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(taskMutex_));
        if (!pendingTasks_.empty()) {
            return false;
        }
    }
    
    // Check if background re-plan is in progress
    if (replanInProgress_.load()) {
        return false;
    }
    
    // Check all robots are IDLE with empty itineraries
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(fleetMutex_));
        for (const auto& [id, agent] : fleetRegistry_) {
            // Check if robot has pending waypoints
            if (!agent.GetItinerary().empty()) {
                return false;
            }
            // Check if robot is still busy
            if (agent.GetStatus() != Layer2::RobotStatus::IDLE) {
                return false;
            }
        }
        
        // Also check drivers are IDLE or ARRIVED
        for (const auto& driver : drivers_) {
            if (driver) {
                auto state = driver->GetState();
                if (state != Layer3::Core::DriverState::IDLE && 
                    state != Layer3::Core::DriverState::ARRIVED) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

// =============================================================================
// MONITORING
// =============================================================================

FleetStats FleetManager::GetStats() const {
    auto now = std::chrono::steady_clock::now();
    FleetStats stats = stats_;
    stats.simulationTime = std::chrono::duration<double>(now - startTime_).count();
    // Compute completed tasks from waypoint counter (2 waypoints = 1 task)
    stats.completedTasks = totalWaypointsVisited_.load() / 2;
    return stats;
}

void FleetManager::PrintRobotStates() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           ROBOT STATES                                    ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  ID │  Status   │ L2 Node │    L3 Position    │ L3 State     │ Itinerary ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════╣\n";
    
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(fleetMutex_));
    
    for (const auto& [id, agent] : fleetRegistry_) {
        const auto* driver = GetRobotDriver(id);
        
        std::cout << "║ " << std::setw(3) << id 
                  << " │ " << std::setw(9) << Layer2::StatusToString(agent.GetStatus())
                  << " │ " << std::setw(7) << agent.GetCurrentNodeId()
                  << " │ ";
        
        if (driver) {
            const auto& pos = driver->GetPosition();
            std::cout << "(" << std::setw(5) << pos.x << "," << std::setw(5) << pos.y << ")";
            std::cout << " │ " << std::setw(12) << driver->GetStateString();
        } else {
            std::cout << "   (N/A)   │      N/A     ";
        }
        
        std::cout << " │ " << std::setw(9) << agent.GetItinerary().size() << " ║\n";
    }
    
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
}

const Layer2::RobotAgent* FleetManager::GetRobotAgent(int robotId) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(fleetMutex_));
    auto it = fleetRegistry_.find(robotId);
    return (it != fleetRegistry_.end()) ? &it->second : nullptr;
}

const Layer3::Core::RobotDriver* FleetManager::GetRobotDriver(int robotId) const {
    for (const auto& driver : drivers_) {
        if (driver && driver->GetRobotId() == robotId) {
            return driver.get();
        }
    }
    return nullptr;
}

std::vector<int> FleetManager::GetPickupNodes() const {
    if (!poiRegistry_) return {};
    return poiRegistry_->GetNodesByType(Layer1::POIType::PICKUP);
}

std::vector<int> FleetManager::GetDropoffNodes() const {
    if (!poiRegistry_) return {};
    return poiRegistry_->GetNodesByType(Layer1::POIType::DROPOFF);
}

// =============================================================================
// THREAD ENTRY POINTS
// =============================================================================

void FleetManager::runMainLoop() {
    std::cout << "[MainLoop] Started (1 Hz)\n";
    
    const auto tickDuration = std::chrono::milliseconds(
        static_cast<int>(config_.warehouseTickMs)
    );
    
    while (running_.load()) {
        auto tickStart = std::chrono::steady_clock::now();
        
        // =====================================================================
        // STEP 1: Check for background re-plan completion (Scenario C)
        // =====================================================================
        if (replanInProgress_.load()) {
            checkBackgroundReplan();
        }
        
        // =====================================================================
        // STEP 2: Process any newly injected tasks (Scenarios B & C)
        // =====================================================================
        processInjectedTasks();
        
        // =====================================================================
        // STEP 3: Check for pending tasks from file load and idle robots
        // (Scenario A: Initial boot or subsequent file loads)
        // =====================================================================
        bool havePendingTasks = false;
        bool haveIdleRobots = false;
        
        {
            std::lock_guard<std::mutex> taskLock(taskMutex_);
            havePendingTasks = !pendingTasks_.empty();
        }
        
        {
            std::lock_guard<std::mutex> fleetLock(fleetMutex_);
            for (const auto& [id, agent] : fleetRegistry_) {
                if (agent.GetStatus() == Layer2::RobotStatus::IDLE && 
                    !agent.GetState().HasPendingGoals()) {
                    haveIdleRobots = true;
                    break;
                }
            }
        }
        
        // Run VRP solver if we have pending tasks and idle robots
        // (This is Scenario A: Full batch solve)
        if (havePendingTasks && haveIdleRobots && !replanInProgress_.load()) {
            std::cout << "\n[MainLoop] ══════════ SCENARIO A: Full VRP Solve ══════════\n";
            runVRPSolver();
            initialSolveComplete_ = true;
        }
        
        stats_.mainLoopCount++;
        
        // SLEEP: In live mode, maintain 1 Hz rate. In batch mode, skip sleep.
        if (!config_.batchMode) {
            auto elapsed = std::chrono::steady_clock::now() - tickStart;
            auto sleepTime = tickDuration - elapsed;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
    
    std::cout << "[MainLoop] Stopped\n";
}

void FleetManager::runFleetLoop() {
    std::cout << "[FleetLoop] Started (" << (config_.batchMode ? "BATCH" : "20 Hz") << ")\n";
    
    const float dt = config_.orcaTickMs / 1000.0f;  // Convert to seconds
    const auto tickDuration = std::chrono::milliseconds(
        static_cast<int>(config_.orcaTickMs)
    );
    
    while (running_.load()) {
        auto tickStart = std::chrono::steady_clock::now();
        
        // Telemetry data to broadcast (collected inside lock, sent outside)
        std::vector<API::RobotTelemetry> telemetry;
        
        // =====================================================================
        // CRITICAL SECTION: Update Physics & Gather Data
        // =====================================================================
        {
            std::lock_guard<std::mutex> lock(fleetMutex_);
            
            // Gather all obstacle data for ORCA
            std::vector<Layer3::Physics::ObstacleData> allObstacles;
            for (const auto& driver : drivers_) {
                if (driver) {
                    allObstacles.push_back(driver->GetObstacleData());
                }
            }
            
            // Update each robot
            for (size_t i = 0; i < drivers_.size(); ++i) {
                if (!drivers_[i]) continue;
                
                int robotId = drivers_[i]->GetRobotId();
                
                // Gather neighbors (exclude self)
                std::vector<Layer3::Physics::ObstacleData> neighbors;
                for (size_t j = 0; j < allObstacles.size(); ++j) {
                    if (i != j) {
                        neighbors.push_back(allObstacles[j]);
                    }
                }
                
                // Update driver physics
                drivers_[i]->UpdateLoop(dt, neighbors);
                
                // Sync L3 position to L2 agent
                syncL3toL2(robotId);
                
                // Check if driver needs a new goal from L2 itinerary
                feedL2toL3(robotId);
                
                // Collect telemetry data for API broadcast
                auto it = fleetRegistry_.find(robotId);
                if (it != fleetRegistry_.end()) {
                    API::RobotTelemetry t;
                    t.id = robotId;
                    t.pos = drivers_[i]->GetPosition();
                    t.velocity = drivers_[i]->GetVelocity();
                    t.status = Layer2::StatusToString(it->second.GetStatus());
                    t.driverState = drivers_[i]->GetStateString();
                    t.battery = 1.0f;  // TODO: Implement battery simulation
                    t.currentNodeId = it->second.GetCurrentNodeId();
                    t.targetNodeId = drivers_[i]->GetGoalNodeId();
                    t.remainingWaypoints = static_cast<int>(it->second.GetItinerary().size());
                    telemetry.push_back(t);
                }
            }
            
            stats_.fleetLoopCount++;
        }
        // =====================================================================
        // END CRITICAL SECTION - Mutex released here
        // =====================================================================
        
        // I/O SECTION: Broadcast telemetry outside the lock (skip in batch mode for speed)
        if (!config_.batchMode) {
            apiService_.BroadcastTelemetry(telemetry);
        }
        
        // SLEEP: In live mode, maintain 20 Hz rate. In batch mode, skip sleep.
        if (!config_.batchMode) {
            auto elapsed = std::chrono::steady_clock::now() - tickStart;
            auto sleepTime = tickDuration - elapsed;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
    
    std::cout << "[FleetLoop] Stopped\n";
}

void FleetManager::runObstacleLoop() {
    std::cout << "[ObstacleLoop] Started (" << (config_.batchMode ? "BATCH" : "1 Hz") << ")\n";
    
    const auto tickDuration = std::chrono::milliseconds(
        static_cast<int>(config_.obstacleTickMs)
    );
    
    while (running_.load()) {
        auto tickStart = std::chrono::steady_clock::now();
        
        // Obstacle data to broadcast (collected inside lock, sent outside)
        std::vector<API::ObstacleInfo> obstacleInfo;
        
        // =====================================================================
        // CRITICAL SECTION: Update Dynamic Obstacles
        // =====================================================================
        {
            std::lock_guard<std::mutex> lock(mapMutex_);
            
            // Placeholder: Update dynamic obstacles
            // In a real system, this would:
            // 1. Get sensor data (e.g., forklift positions)
            // 2. Convert to DynamicObstacle objects
            // 3. Call dynamicMap_->Update(obstacles, *staticMap_)
            
            // Future: dynamicMap_->Update(obstacles, *staticMap_);
            
            // Collect obstacle data for API broadcast
            // For now, this is empty. When dynamic obstacles are added:
            // for (const auto& obs : activeObstacles) {
            //     obstacleInfo.push_back({obs.topLeft, obs.width, obs.height, obs.type});
            // }
        }
        // =====================================================================
        // END CRITICAL SECTION - Mutex released here
        // =====================================================================
        
        // I/O SECTION: Broadcast obstacles outside the lock (skip in batch mode)
        if (!config_.batchMode) {
            apiService_.BroadcastObstacles(obstacleInfo);
        }
        
        stats_.obstacleLoopCount++;
        
        // SLEEP: In live mode, maintain 1 Hz rate. In batch mode, skip sleep.
        if (!config_.batchMode) {
            auto elapsed = std::chrono::steady_clock::now() - tickStart;
            auto sleepTime = tickDuration - elapsed;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
    
    std::cout << "[ObstacleLoop] Stopped\n";
}

// =============================================================================
// BRIDGE LOGIC
// =============================================================================

void FleetManager::syncL3toL2(int robotId) {
    // Find the driver
    Layer3::Core::RobotDriver* driver = nullptr;
    for (auto& d : drivers_) {
        if (d && d->GetRobotId() == robotId) {
            driver = d.get();
            break;
        }
    }
    
    if (!driver) return;
    
    // Get driver position
    const auto& pos = driver->GetPosition();
    
    // Find nearest NavMesh node
    int nearestNode = findNearestNode(pos);
    
    // Update Layer 2 agent
    auto it = fleetRegistry_.find(robotId);
    if (it != fleetRegistry_.end()) {
        it->second.SetCurrentNodeId(nearestNode);
        
        // Update status based on driver state
        auto driverState = driver->GetState();
        if (driverState == Layer3::Core::DriverState::MOVING ||
            driverState == Layer3::Core::DriverState::COMPUTING_PATH) {
            it->second.SetStatus(Layer2::RobotStatus::BUSY);
        } else if (driverState == Layer3::Core::DriverState::IDLE ||
                   driverState == Layer3::Core::DriverState::ARRIVED) {
            // Only set IDLE if no more itinerary items
            if (!it->second.GetState().HasPendingGoals()) {
                it->second.SetStatus(Layer2::RobotStatus::IDLE);
            }
        }
    }
}

void FleetManager::feedL2toL3(int robotId) {
    // Find the driver
    Layer3::Core::RobotDriver* driver = nullptr;
    for (auto& d : drivers_) {
        if (d && d->GetRobotId() == robotId) {
            driver = d.get();
            break;
        }
    }
    
    if (!driver) return;
    
    // Check if driver is idle or arrived
    auto driverState = driver->GetState();
    if (driverState != Layer3::Core::DriverState::IDLE &&
        driverState != Layer3::Core::DriverState::ARRIVED) {
        return;  // Driver is busy
    }
    
    // Check if L2 agent has pending goals
    auto it = fleetRegistry_.find(robotId);
    if (it == fleetRegistry_.end()) return;
    
    auto& agent = it->second;
    if (!agent.GetState().HasPendingGoals()) {
        return;  // No pending goals
    }
    
    // Pop next goal from L2 itinerary
    int nextGoal = agent.GetMutableState().PopNextGoal();
    
    if (nextGoal >= 0) {
        std::cout << "[Bridge] Robot " << robotId << ": L2→L3 SetGoal(" << nextGoal << ")\n";
        driver->SetGoal(nextGoal);
        agent.SetStatus(Layer2::RobotStatus::BUSY);
    }
}

void FleetManager::runVRPSolver() {
    std::cout << "\n[MainLoop] Running VRP solver...\n";
    
    // Get tasks and robots
    std::vector<Layer2::Task> tasks;
    std::vector<Layer2::RobotAgent> robots;
    
    {
        std::lock_guard<std::mutex> taskLock(taskMutex_);
        tasks = pendingTasks_;
    }
    
    {
        std::lock_guard<std::mutex> fleetLock(fleetMutex_);
        for (auto& [id, agent] : fleetRegistry_) {
            robots.push_back(agent);
        }
    }
    
    if (tasks.empty()) {
        std::cout << "[MainLoop] No pending tasks\n";
        return;
    }
    
    // Run VRP solver
    auto result = vrpSolver_->Solve(tasks, robots, *costMatrix_);
    
    if (!result.isFeasible) {
        std::cerr << "[MainLoop] VRP solver returned infeasible solution!\n";
        return;
    }
    
    result.Print();
    
    // Assign itineraries to robots
    {
        std::lock_guard<std::mutex> fleetLock(fleetMutex_);
        
        for (const auto& [robotId, itinerary] : result.robotItineraries) {
            auto it = fleetRegistry_.find(robotId);
            if (it != fleetRegistry_.end()) {
                it->second.AssignItinerary(itinerary);
                std::cout << "[MainLoop] Robot " << robotId << " assigned " 
                          << itinerary.size() << " waypoints\n";
            }
        }
    }
    
    // Clear pending tasks (they've been assigned)
    {
        std::lock_guard<std::mutex> taskLock(taskMutex_);
        pendingTasks_.clear();
        stats_.pendingTasks = 0;
    }
}

// =============================================================================
// DYNAMIC SCHEDULING (Scenarios B & C)
// =============================================================================

void FleetManager::processInjectedTasks() {
    // Drain the injection queue
    std::vector<Layer2::Task> newTasks;
    {
        std::lock_guard<std::mutex> lock(injectionMutex_);
        while (!injectionQueue_.empty()) {
            newTasks.push_back(injectionQueue_.front());
            injectionQueue_.pop();
        }
    }
    
    if (newTasks.empty()) {
        return;
    }
    
    std::cout << "\n[MainLoop] Processing " << newTasks.size() << " injected tasks...\n";
    
    // Update stats
    stats_.totalTasks += static_cast<int>(newTasks.size());
    
    // Decision: Scenario B (cheap insertion) or Scenario C (background re-plan)?
    if (static_cast<int>(newTasks.size()) <= config_.batchThreshold) {
        // SCENARIO B: Small batch - use cheap insertion
        std::cout << "[MainLoop] ══════════ SCENARIO B: Cheap Insertion ══════════\n";
        std::cout << "[MainLoop] " << newTasks.size() << " tasks <= threshold (" 
                  << config_.batchThreshold << "), using insertion heuristic\n";
        runCheapInsertion(newTasks);
    } else {
        // SCENARIO C: Large batch - use background re-planning
        std::cout << "[MainLoop] ══════════ SCENARIO C: Background Re-plan ══════════\n";
        std::cout << "[MainLoop] " << newTasks.size() << " tasks > threshold (" 
                  << config_.batchThreshold << "), launching background solver\n";
        
        // STEP 1: Assign "starter" tasks immediately so robots don't stay idle
        // Each robot gets a few tasks to work on while the solver optimizes
        std::vector<Layer2::Task> starterTasks;
        std::vector<Layer2::Task> tasksForSolver;
        
        int numRobots = static_cast<int>(fleetRegistry_.size());
        int totalStarterTasks = numRobots * config_.starterTasksPerRobot;
        
        // Split: first N tasks go to starters, rest go to solver
        for (size_t i = 0; i < newTasks.size(); ++i) {
            if (static_cast<int>(i) < totalStarterTasks) {
                starterTasks.push_back(newTasks[i]);
            } else {
                tasksForSolver.push_back(newTasks[i]);
            }
        }
        
        std::cout << "[MainLoop] Splitting " << newTasks.size() << " tasks:\n";
        std::cout << "           - Starter tasks (immediate): " << starterTasks.size() 
                  << " (" << config_.starterTasksPerRobot << " per robot)\n";
        std::cout << "           - For background solver: " << tasksForSolver.size() << "\n";
        
        // Assign starter tasks using cheap insertion (immediate, no waiting)
        if (!starterTasks.empty()) {
            std::cout << "[MainLoop] Assigning starter tasks to keep robots busy...\n";
            runCheapInsertion(starterTasks);
        }
        
        // STEP 2: Collect remaining work for the background solver
        // Only if there are tasks left to optimize
        if (!tasksForSolver.empty()) {
            // Launch background re-plan for the remaining tasks only
            // (Starter tasks are already assigned and won't be re-optimized)
            launchBackgroundReplan(tasksForSolver);
        } else {
            std::cout << "[MainLoop] All tasks assigned as starters, no background solve needed\n";
        }
    }
}

void FleetManager::runCheapInsertion(const std::vector<Layer2::Task>& tasks) {
    std::lock_guard<std::mutex> lock(fleetMutex_);
    
    for (const auto& task : tasks) {
        int bestRobotId = -1;
        float bestCost = std::numeric_limits<float>::max();
        
        // Find the robot with the cheapest insertion cost
        for (const auto& [robotId, agent] : fleetRegistry_) {
            float cost = calculateInsertionCost(agent, task);
            if (cost < bestCost) {
                bestCost = cost;
                bestRobotId = robotId;
            }
        }
        
        if (bestRobotId >= 0) {
            // Insert task at the end of this robot's itinerary
            auto it = fleetRegistry_.find(bestRobotId);
            if (it != fleetRegistry_.end()) {
                // Get mutable itinerary and append pickup + dropoff
                auto& state = it->second.GetMutableState();
                state.currentItinerary.push_back(task.sourceNode);
                state.currentItinerary.push_back(task.destinationNode);
                
                std::cout << "[Insertion] Task " << task.taskId << " assigned to Robot " 
                          << bestRobotId << " (extra cost: " << std::fixed 
                          << std::setprecision(1) << bestCost << " px)\n";
            }
        } else {
            // No robot available, add to pending tasks for next VRP solve
            std::lock_guard<std::mutex> taskLock(taskMutex_);
            pendingTasks_.push_back(task);
            stats_.pendingTasks++;
            std::cout << "[Insertion] No robot available for task " << task.taskId 
                      << ", queued for next solve\n";
        }
    }
}

void FleetManager::launchBackgroundReplan(const std::vector<Layer2::Task>& tasks) {
    if (replanInProgress_.load()) {
        std::cout << "[Replan] Background re-plan already in progress, queuing tasks...\n";
        std::lock_guard<std::mutex> lock(taskMutex_);
        for (const auto& task : tasks) {
            pendingTasks_.push_back(task);
        }
        stats_.pendingTasks += static_cast<int>(tasks.size());
        return;
    }
    
    replanInProgress_ = true;
    
    // Capture current robot states for the solver
    std::vector<Layer2::RobotAgent> robots;
    {
        std::lock_guard<std::mutex> lock(fleetMutex_);
        for (const auto& [id, agent] : fleetRegistry_) {
            robots.push_back(agent);
        }
    }
    
    // Launch async solver
    std::cout << "[Replan] Launching VRP solver in background thread...\n";
    std::cout << "[Replan] Tasks: " << tasks.size() << ", Robots: " << robots.size() << "\n";
    
    // Capture necessary data for the lambda
    auto* solver = vrpSolver_.get();
    auto* costs = costMatrix_.get();
    
    // Use mutable lambda since Solve requires non-const robots reference
    replanFuture_ = std::async(std::launch::async, [solver, costs, tasks, robots]() mutable {
        return solver->Solve(tasks, robots, *costs);
    });
    
    std::cout << "[Replan] Background solver started (ETA: ~" 
              << config_.estimatedReplanTimeMs << " ms)\n";
}

void FleetManager::checkBackgroundReplan() {
    if (!replanInProgress_.load()) {
        return;
    }
    
    // Check if the future is ready (non-blocking)
    if (replanFuture_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        // Still computing - check if any robots should wait
        // (Scenario C: Smart wait logic)
        std::lock_guard<std::mutex> lock(fleetMutex_);
        
        for (auto& [id, agent] : fleetRegistry_) {
            if (agent.GetStatus() == Layer2::RobotStatus::IDLE && 
                !agent.GetState().HasPendingGoals()) {
                // Robot is idle and has no work - make it wait for the re-plan
                // (In a real system, we'd check estimated remaining time vs replan time)
                // For now, just log that the robot is waiting
                if (std::find(robotsWaitingForReplan_.begin(), 
                             robotsWaitingForReplan_.end(), id) == robotsWaitingForReplan_.end()) {
                    robotsWaitingForReplan_.push_back(id);
                    std::cout << "[Replan] Robot " << id << " waiting for re-plan to complete\n";
                }
            }
        }
        return;
    }
    
    // Re-plan is ready - apply the new itineraries!
    std::cout << "\n[Replan] ══════════ Background re-plan complete! ══════════\n";
    
    try {
        Layer2::VRPResult result = replanFuture_.get();
        
        if (!result.isFeasible) {
            std::cerr << "[Replan] WARNING: Solver returned infeasible solution!\n";
        } else {
            result.Print();
            
            // APPEND MODE: Add optimized tasks to existing itineraries
            // (Starter tasks are already assigned and being worked on)
            {
                std::lock_guard<std::mutex> lock(fleetMutex_);
                
                for (const auto& [robotId, newItinerary] : result.robotItineraries) {
                    auto it = fleetRegistry_.find(robotId);
                    if (it != fleetRegistry_.end()) {
                        // Get current itinerary (includes starter tasks)
                        auto& state = it->second.GetMutableState();
                        size_t existingSize = state.currentItinerary.size();
                        
                        // Append new optimized waypoints to the end
                        for (int waypoint : newItinerary) {
                            state.currentItinerary.push_back(waypoint);
                        }
                        
                        std::cout << "[Replan] Robot " << robotId << ": appended " 
                                  << newItinerary.size() << " waypoints (had " 
                                  << existingSize << ", now " 
                                  << state.currentItinerary.size() << ")\n";
                    }
                }
            }
            
            // Clear waiting robots list
            robotsWaitingForReplan_.clear();
            
            std::cout << "[Replan] Optimized tasks appended to all robot itineraries.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[Replan] ERROR: Background solver threw exception: " << e.what() << "\n";
    }
    
    replanInProgress_ = false;
}

float FleetManager::calculateInsertionCost(const Layer2::RobotAgent& robot, 
                                           const Layer2::Task& task) const {
    // Get the robot's current position (or last itinerary node)
    int lastNode = robot.GetCurrentNodeId();
    const auto& itinerary = robot.GetItinerary();
    if (!itinerary.empty()) {
        lastNode = itinerary.back();
    }
    
    // Cost = distance from last node to pickup + distance from pickup to dropoff
    float costToPickup = costMatrix_->GetCost(lastNode, task.sourceNode);
    float costPickupToDropoff = costMatrix_->GetCost(task.sourceNode, task.destinationNode);
    
    // If costs are invalid (unreachable), return infinity
    if (costToPickup < 0 || costPickupToDropoff < 0) {
        return std::numeric_limits<float>::max();
    }
    
    return costToPickup + costPickupToDropoff;
}

int FleetManager::estimateRobotRemainingTimeMs(int robotId) const {
    // Estimate based on itinerary length and robot speed
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(fleetMutex_));
    
    auto it = fleetRegistry_.find(robotId);
    if (it == fleetRegistry_.end()) {
        return 0;
    }
    
    const auto& itinerary = it->second.GetItinerary();
    if (itinerary.empty()) {
        return 0;
    }
    
    // Rough estimate: assume average 500 pixels per waypoint at 16 pixels/s
    float avgPixelsPerWaypoint = 500.0f;
    float speedPixelsPerSec = config_.robotSpeedMps * 10.0f;  // Convert to decimeters/s
    
    float totalPixels = static_cast<float>(itinerary.size()) * avgPixelsPerWaypoint;
    float timeSeconds = totalPixels / speedPixelsPerSec;
    
    return static_cast<int>(timeSeconds * 1000.0f);
}

int FleetManager::findNearestNode(const Common::Coordinates& pos) const {
    // Use NavMesh's built-in method
    return navMesh_->GetNodeIdAt(pos);
}

} // namespace Backend
