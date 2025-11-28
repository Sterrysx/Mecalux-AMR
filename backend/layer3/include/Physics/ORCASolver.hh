/**
 * @file ORCASolver.hh
 * @brief Optimal Reciprocal Collision Avoidance solver
 * 
 * Implements local collision avoidance using velocity obstacles.
 * For this iteration, implements basic repulsion/stop-and-wait logic.
 */

#ifndef LAYER3_PHYSICS_ORCASOLVER_HH
#define LAYER3_PHYSICS_ORCASOLVER_HH

#include <vector>
#include <cmath>

#include "Vector2.hh"
#include "Physics/ObstacleData.hh"

namespace Backend {
namespace Layer3 {
namespace Physics {

/**
 * @brief Configuration parameters for ORCA.
 */
struct ORCAConfig {
    double safetyMargin;          ///< Extra clearance beyond robot radius (pixels)
    double timeHorizon;           ///< Look-ahead time for collision prediction (seconds)
    double maxSpeed;              ///< Maximum allowed velocity magnitude (pixels/second)
    double responsiveness;        ///< How quickly to respond to threats (0-1)
    double stopDistance;          ///< Distance at which to stop completely (pixels)
    double slowdownDistance;      ///< Distance at which to start slowing down (pixels)
    
    /**
     * @brief Default constructor with sensible defaults.
     */
    ORCAConfig()
        : safetyMargin(5.0)       // 0.5m at DECIMETERS
        , timeHorizon(2.0)         // Look 2 seconds ahead
        , maxSpeed(16.0)           // 1.6 m/s at DECIMETERS = 16 pixels/s
        , responsiveness(0.5)      // 50% velocity correction per tick
        , stopDistance(10.0)       // 1m at DECIMETERS
        , slowdownDistance(30.0)   // 3m at DECIMETERS
    {}
};

/**
 * @brief ORCA collision avoidance solver.
 * 
 * This implementation uses a simplified approach:
 * 1. Basic Repulsion: Push velocity away from nearby obstacles
 * 2. Stop-and-Wait: Stop completely if collision is imminent
 * 
 * The full ORCA algorithm constructs half-plane constraints from
 * velocity obstacles, but this simpler approach is sufficient
 * to prove the architecture.
 * 
 * Usage:
 *   ORCASolver solver;
 *   Vector2 safeVel = solver.CalculateSafeVelocity(myData, neighbors, preferredVel);
 */
class ORCASolver {
private:
    ORCAConfig config_;

public:
    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Default constructor.
     */
    ORCASolver() : config_() {}
    
    /**
     * @brief Construct with custom configuration.
     */
    explicit ORCASolver(const ORCAConfig& config) : config_(config) {}

    // =========================================================================
    // CONFIGURATION
    // =========================================================================
    
    /**
     * @brief Get current configuration.
     */
    const ORCAConfig& GetConfig() const { return config_; }
    
    /**
     * @brief Update configuration.
     */
    void SetConfig(const ORCAConfig& config) { config_ = config; }

    // =========================================================================
    // MAIN INTERFACE
    // =========================================================================
    
    /**
     * @brief Calculate a collision-free velocity.
     * 
     * Given the robot's current state, nearby obstacles, and desired velocity,
     * compute a safe velocity that avoids collisions.
     * 
     * @param me Robot's obstacle data (position, radius)
     * @param neighbors List of nearby obstacles
     * @param preferredVelocity Desired velocity (direction to goal)
     * @return Safe velocity vector
     */
    Vector2 CalculateSafeVelocity(
        const ObstacleData& me,
        const std::vector<ObstacleData>& neighbors,
        const Vector2& preferredVelocity
    ) const;

private:
    // =========================================================================
    // COLLISION DETECTION
    // =========================================================================
    
    /**
     * @brief Find the closest obstacle distance.
     */
    double FindClosestObstacleDistance(
        const ObstacleData& me,
        const std::vector<ObstacleData>& neighbors
    ) const;
    
    /**
     * @brief Check if a velocity will cause collision within time horizon.
     */
    bool WillCollide(
        const ObstacleData& me,
        const ObstacleData& neighbor,
        const Vector2& velocity,
        double timeHorizon
    ) const;

    // =========================================================================
    // VELOCITY MODIFICATION
    // =========================================================================
    
    /**
     * @brief Apply repulsion forces from all neighbors.
     */
    Vector2 ApplyRepulsion(
        const ObstacleData& me,
        const std::vector<ObstacleData>& neighbors,
        const Vector2& velocity
    ) const;
    
    /**
     * @brief Scale velocity based on closest obstacle.
     */
    Vector2 ApplySlowdown(
        const Vector2& velocity,
        double closestDistance,
        double combinedRadius
    ) const;
    
    /**
     * @brief Clamp velocity to maximum speed.
     */
    Vector2 ClampSpeed(const Vector2& velocity) const;
};

} // namespace Physics
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_PHYSICS_ORCASOLVER_HH
