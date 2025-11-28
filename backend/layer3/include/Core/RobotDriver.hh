/**
 * @file RobotDriver.hh
 * @brief Core robot driver for path following and movement control
 * 
 * The RobotDriver is the "Tactical Brain" - it receives high-level goals
 * (Node IDs) from Layer 2 and converts them into smooth, real-time movement.
 */

#ifndef LAYER3_CORE_ROBOTDRIVER_HH
#define LAYER3_CORE_ROBOTDRIVER_HH

#include <vector>
#include <functional>

#include "Vector2.hh"
#include "Pathfinding/PathfindingService.hh"
#include "Physics/ORCASolver.hh"
#include "Physics/ObstacleData.hh"
#include "Coordinates.hh"
#include "NavMesh.hh"

namespace Backend {
namespace Layer3 {
namespace Core {

/**
 * @brief Robot driver state enumeration.
 */
enum class DriverState {
    IDLE,               ///< No goal, stationary
    COMPUTING_PATH,     ///< Waiting for path computation
    MOVING,             ///< Following path to goal
    ARRIVED,            ///< Reached goal
    STUCK,              ///< Cannot reach goal
    COLLISION_WAIT      ///< Waiting for obstacle to clear
};

/**
 * @brief Configuration for robot driver.
 */
struct DriverConfig {
    double maxSpeed;              ///< Maximum speed (pixels/second)
    double acceleration;          ///< Acceleration rate (pixels/second²)
    double waypointThreshold;     ///< Distance to consider waypoint reached (pixels)
    double goalThreshold;         ///< Distance to consider goal reached (pixels)
    double robotRadius;           ///< Robot collision radius (pixels)
    
    /**
     * @brief Default configuration for DECIMETERS resolution.
     */
    DriverConfig()
        : maxSpeed(16.0)           // 1.6 m/s = 16 pixels/s at DECIMETERS
        , acceleration(8.0)        // 0.8 m/s² = 8 pixels/s² at DECIMETERS
        , waypointThreshold(5.0)   // 0.5m at DECIMETERS
        , goalThreshold(3.0)       // 0.3m at DECIMETERS
        , robotRadius(3.0)         // 0.3m at DECIMETERS
    {}
};

/**
 * @brief Callback when goal is reached.
 */
using GoalReachedCallback = std::function<void(int robotId, int goalNodeId)>;

/**
 * @brief Core robot driver class.
 * 
 * Responsibilities:
 * 1. Receive goal node IDs from Layer 2
 * 2. Request paths from PathfindingService
 * 3. Follow path with smooth velocity control
 * 4. Use ORCA for collision avoidance with neighbors
 * 5. Report position/velocity updates to simulator
 * 
 * Usage:
 *   RobotDriver driver(0, startPos, navMesh);
 *   driver.SetGoal(targetNodeId);
 *   
 *   // In physics loop:
 *   driver.UpdateLoop(0.05f, neighbors);
 */
class RobotDriver {
private:
    // Identity
    int robotId_;
    
    // Configuration
    DriverConfig config_;
    
    // State
    DriverState state_;
    Vector2 precisePosition_;                   // High-precision position for physics
    Backend::Common::Coordinates currentPosition_;  // Integer position for grid lookups
    Vector2 currentVelocity_;
    double currentSpeed_;
    
    // Path following
    std::vector<Backend::Common::Coordinates> currentPath_;
    size_t pathIndex_;
    int currentGoalNodeId_;
    
    // Reference to NavMesh for node lookups
    const Backend::Layer1::NavMesh* navMesh_;
    
    // ORCA solver for collision avoidance
    Physics::ORCASolver orcaSolver_;
    
    // Callbacks
    GoalReachedCallback onGoalReached_;

public:
    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Construct a robot driver.
     * 
     * @param id Unique robot identifier
     * @param startPosition Initial position (pixels)
     * @param navMesh Reference to NavMesh for node lookups
     * @param config Driver configuration
     */
    RobotDriver(
        int id,
        const Backend::Common::Coordinates& startPosition,
        const Backend::Layer1::NavMesh& navMesh,
        const DriverConfig& config = DriverConfig()
    );
    
    /**
     * @brief Default constructor (for containers).
     */
    RobotDriver();

    // =========================================================================
    // GOAL SETTING
    // =========================================================================
    
    /**
     * @brief Set a new goal node.
     * 
     * Looks up the node's coordinates from NavMesh and requests a path.
     * 
     * @param nodeId Target NavMesh node ID
     * @return true if path request was initiated
     */
    bool SetGoal(int nodeId);
    
    /**
     * @brief Set a new goal by coordinates.
     * 
     * Directly requests a path to the given position.
     * 
     * @param target Target position (pixels)
     * @return true if path request was initiated
     */
    bool SetGoalPosition(const Backend::Common::Coordinates& target);
    
    /**
     * @brief Cancel current goal and stop.
     */
    void CancelGoal();
    
    /**
     * @brief Check if robot has an active goal.
     */
    bool HasGoal() const;
    
    /**
     * @brief Get current goal node ID.
     */
    int GetGoalNodeId() const { return currentGoalNodeId_; }

    // =========================================================================
    // UPDATE LOOP
    // =========================================================================
    
    /**
     * @brief Main update loop - called every physics tick.
     * 
     * This method:
     * 1. Calculates preferred velocity towards next waypoint
     * 2. Applies ORCA collision avoidance
     * 3. Updates position based on safe velocity
     * 
     * @param dt Delta time in seconds (e.g., 0.05 for 50ms tick)
     * @param neighbors Other robots/obstacles for collision avoidance
     */
    void UpdateLoop(float dt, const std::vector<Physics::ObstacleData>& neighbors);

    // =========================================================================
    // GETTERS - State
    // =========================================================================
    
    int GetRobotId() const { return robotId_; }
    DriverState GetState() const { return state_; }
    const Backend::Common::Coordinates& GetPosition() const { return currentPosition_; }
    Vector2 GetVelocity() const { return currentVelocity_; }
    double GetSpeed() const { return currentSpeed_; }
    double GetRadius() const { return config_.robotRadius; }
    
    /**
     * @brief Get obstacle data for this robot (for ORCA).
     */
    Physics::ObstacleData GetObstacleData() const;
    
    /**
     * @brief Get remaining path length.
     */
    double GetRemainingPathLength() const;
    
    /**
     * @brief Get ETA to goal in seconds.
     */
    double GetETA() const;
    
    /**
     * @brief Get current path (read-only).
     */
    const std::vector<Backend::Common::Coordinates>& GetPath() const { return currentPath_; }

    // =========================================================================
    // SETTERS
    // =========================================================================
    
    /**
     * @brief Set callback for goal reached.
     */
    void SetOnGoalReached(GoalReachedCallback callback) {
        onGoalReached_ = callback;
    }
    
    /**
     * @brief Update configuration.
     */
    void SetConfig(const DriverConfig& config) { config_ = config; }
    
    /**
     * @brief Set ORCA configuration.
     */
    void SetORCAConfig(const Physics::ORCAConfig& config) {
        orcaSolver_.SetConfig(config);
    }
    
    /**
     * @brief Directly set position (for teleporting/initialization).
     */
    void SetPosition(const Backend::Common::Coordinates& pos) {
        currentPosition_ = pos;
    }

    // =========================================================================
    // STATE STRING
    // =========================================================================
    
    /**
     * @brief Get state as string for debugging.
     */
    std::string GetStateString() const;

private:
    // =========================================================================
    // INTERNAL METHODS
    // =========================================================================
    
    /**
     * @brief Calculate preferred velocity towards next waypoint.
     */
    Vector2 CalculatePreferredVelocity() const;
    
    /**
     * @brief Check if current waypoint is reached.
     */
    bool IsWaypointReached() const;
    
    /**
     * @brief Check if final goal is reached.
     */
    bool IsGoalReached() const;
    
    /**
     * @brief Advance to next waypoint.
     */
    void AdvanceWaypoint();
    
    /**
     * @brief Handle path completion callback.
     */
    void OnPathReceived(const Pathfinding::PathResult& result);
    
    /**
     * @brief Apply velocity with acceleration limits.
     */
    Vector2 ApplyAcceleration(const Vector2& targetVelocity, float dt) const;
};

} // namespace Core
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_CORE_ROBOTDRIVER_HH
