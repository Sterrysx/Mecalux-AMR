/**
 * @file FastLoopManager.hh
 * @brief High-frequency physics loop manager
 * 
 * Manages all RobotDrivers and runs the physics simulation loop.
 */

#ifndef LAYER3_CORE_FASTLOOPMANAGER_HH
#define LAYER3_CORE_FASTLOOPMANAGER_HH

#include <vector>
#include <memory>
#include <chrono>
#include <functional>

#include "Core/RobotDriver.hh"
#include "Physics/ObstacleData.hh"
#include "Coordinates.hh"

namespace Backend {
namespace Layer3 {
namespace Core {

/**
 * @brief Statistics for the physics loop.
 */
struct LoopStats {
    int tickCount;                  ///< Total ticks executed
    double totalTimeMs;             ///< Total simulation time (ms)
    double avgTickTimeMs;           ///< Average tick computation time (ms)
    double maxTickTimeMs;           ///< Maximum tick computation time (ms)
    int totalCollisionAvoidances;   ///< Number of collision avoidance events
    
    LoopStats()
        : tickCount(0)
        , totalTimeMs(0.0)
        , avgTickTimeMs(0.0)
        , maxTickTimeMs(0.0)
        , totalCollisionAvoidances(0) {}
};

/**
 * @brief Callback for each physics tick.
 * 
 * Called after all robots are updated.
 * Parameters: tick number, delta time
 */
using TickCallback = std::function<void(int, float)>;

/**
 * @brief Fast loop manager for robot physics.
 * 
 * Responsibilities:
 * 1. Maintain list of all RobotDrivers
 * 2. Run physics loop at configurable frequency (default 50ms)
 * 3. Gather neighbor data for each robot
 * 4. Call UpdateLoop on each driver
 * 5. Track statistics
 * 
 * Usage:
 *   FastLoopManager manager;
 *   manager.AddRobot(robot1);
 *   manager.AddRobot(robot2);
 *   
 *   // Run for 5 seconds
 *   manager.RunForDuration(5.0f);
 *   
 *   // Or run specific number of ticks
 *   manager.RunTicks(100);
 */
class FastLoopManager {
private:
    // Robots being managed
    std::vector<RobotDriver> robots_;
    
    // Configuration
    float tickDuration_;      ///< Duration of each tick (seconds)
    double neighborRadius_;   ///< Radius for neighbor detection (pixels)
    
    // Statistics
    LoopStats stats_;
    
    // Callbacks
    TickCallback onTick_;
    
    // Simulation state
    bool isRunning_;
    double simulationTime_;   ///< Total simulated time (seconds)

public:
    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Construct with default 50ms tick.
     */
    FastLoopManager();
    
    /**
     * @brief Construct with custom tick duration.
     * 
     * @param tickDurationMs Tick duration in milliseconds
     */
    explicit FastLoopManager(float tickDurationMs);

    // =========================================================================
    // ROBOT MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Add a robot to the simulation.
     * 
     * @param robot Robot driver to add
     * @return Index of the added robot
     */
    int AddRobot(const RobotDriver& robot);
    
    /**
     * @brief Add a robot by move semantics.
     */
    int AddRobot(RobotDriver&& robot);
    
    /**
     * @brief Create and add a robot.
     * 
     * @param id Robot ID
     * @param position Starting position
     * @param navMesh NavMesh reference
     * @return Reference to the created robot
     */
    RobotDriver& CreateRobot(
        int id,
        const Backend::Common::Coordinates& position,
        const Backend::Layer1::NavMesh& navMesh
    );
    
    /**
     * @brief Get robot by ID.
     * 
     * @return Pointer to robot, or nullptr if not found
     */
    RobotDriver* GetRobot(int robotId);
    
    /**
     * @brief Get robot by index.
     */
    RobotDriver& GetRobotByIndex(size_t index);
    
    /**
     * @brief Get all robots.
     */
    std::vector<RobotDriver>& GetRobots() { return robots_; }
    const std::vector<RobotDriver>& GetRobots() const { return robots_; }
    
    /**
     * @brief Get number of robots.
     */
    size_t GetRobotCount() const { return robots_.size(); }
    
    /**
     * @brief Remove a robot by ID.
     */
    bool RemoveRobot(int robotId);
    
    /**
     * @brief Clear all robots.
     */
    void ClearRobots();

    // =========================================================================
    // SIMULATION CONTROL
    // =========================================================================
    
    /**
     * @brief Run the physics loop for a specific duration.
     * 
     * @param durationSeconds How long to simulate (seconds)
     * @return Number of ticks executed
     */
    int RunForDuration(float durationSeconds);
    
    /**
     * @brief Run a specific number of ticks.
     * 
     * @param numTicks Number of physics ticks to run
     */
    void RunTicks(int numTicks);
    
    /**
     * @brief Run a single physics tick.
     * 
     * Use this for external control of the loop.
     */
    void RunSingleTick();
    
    /**
     * @brief Stop the simulation.
     */
    void Stop();
    
    /**
     * @brief Check if simulation is running.
     */
    bool IsRunning() const { return isRunning_; }

    // =========================================================================
    // CONFIGURATION
    // =========================================================================
    
    /**
     * @brief Set tick duration.
     * 
     * @param durationMs Tick duration in milliseconds
     */
    void SetTickDuration(float durationMs);
    
    /**
     * @brief Get tick duration in seconds.
     */
    float GetTickDuration() const { return tickDuration_; }
    
    /**
     * @brief Set neighbor detection radius.
     */
    void SetNeighborRadius(double radius) { neighborRadius_ = radius; }
    
    /**
     * @brief Set tick callback.
     */
    void SetOnTick(TickCallback callback) { onTick_ = callback; }

    // =========================================================================
    // STATISTICS
    // =========================================================================
    
    /**
     * @brief Get loop statistics.
     */
    const LoopStats& GetStats() const { return stats_; }
    
    /**
     * @brief Reset statistics.
     */
    void ResetStats();
    
    /**
     * @brief Get total simulated time.
     */
    double GetSimulationTime() const { return simulationTime_; }

    // =========================================================================
    // DEBUG
    // =========================================================================
    
    /**
     * @brief Print state of all robots.
     */
    void PrintRobotStates() const;

private:
    // =========================================================================
    // INTERNAL METHODS
    // =========================================================================
    
    /**
     * @brief Gather neighbors for a specific robot.
     */
    std::vector<Physics::ObstacleData> GatherNeighbors(size_t robotIndex) const;
    
    /**
     * @brief Execute one physics tick for all robots.
     */
    void ExecuteTick();
};

} // namespace Core
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_CORE_FASTLOOPMANAGER_HH
