/**
 * @file ObstacleData.hh
 * @brief Data structure for representing dynamic obstacles
 * 
 * Used by the ORCA solver for collision avoidance calculations.
 */

#ifndef LAYER3_PHYSICS_OBSTACLEDATA_HH
#define LAYER3_PHYSICS_OBSTACLEDATA_HH

#include "Vector2.hh"
#include "Coordinates.hh"

namespace Backend {
namespace Layer3 {
namespace Physics {

/**
 * @brief Represents a dynamic obstacle (another robot or moving object).
 * 
 * Contains all information needed for ORCA velocity obstacle calculations:
 * - Position in world coordinates
 * - Current velocity vector
 * - Collision radius
 */
struct ObstacleData {
    int id;                                  ///< Unique identifier (-1 for static)
    Backend::Common::Coordinates position;   ///< Current position (pixels)
    Vector2 velocity;                        ///< Current velocity (pixels/second)
    double radius;                           ///< Collision radius (pixels)
    
    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Default constructor.
     */
    ObstacleData()
        : id(-1)
        , position{0, 0}
        , velocity()
        , radius(0.0) {}
    
    /**
     * @brief Full constructor.
     */
    ObstacleData(int obstacleId, 
                 const Backend::Common::Coordinates& pos,
                 const Vector2& vel,
                 double r)
        : id(obstacleId)
        , position{pos.x, pos.y}
        , velocity(vel)
        , radius(r) {}
    
    /**
     * @brief Construct static obstacle (zero velocity).
     */
    ObstacleData(const Backend::Common::Coordinates& pos, double r)
        : id(-1)
        , position{pos.x, pos.y}
        , velocity()
        , radius(r) {}

    // =========================================================================
    // HELPERS
    // =========================================================================
    
    /**
     * @brief Get position as Vector2.
     */
    Vector2 GetPositionVec() const {
        return Vector2(position.x, position.y);
    }
    
    /**
     * @brief Check if this is a static obstacle.
     */
    bool IsStatic() const {
        return velocity.MagnitudeSquared() < 1e-6;
    }
    
    /**
     * @brief Distance to another obstacle (center to center).
     */
    double DistanceTo(const ObstacleData& other) const {
        double dx = position.x - other.position.x;
        double dy = position.y - other.position.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief Combined radius (for collision detection).
     */
    double CombinedRadius(const ObstacleData& other) const {
        return radius + other.radius;
    }
    
    /**
     * @brief Check if colliding with another obstacle.
     */
    bool IsCollidingWith(const ObstacleData& other) const {
        return DistanceTo(other) < CombinedRadius(other);
    }
    
    /**
     * @brief Predict position after time dt.
     */
    Backend::Common::Coordinates PredictPosition(double dt) const {
        Backend::Common::Coordinates result;
        result.x = position.x + static_cast<int>(velocity.x * dt);
        result.y = position.y + static_cast<int>(velocity.y * dt);
        return result;
    }
};

/**
 * @brief Robot-specific obstacle data with additional state.
 */
struct RobotObstacle : public ObstacleData {
    int robotId;              ///< Robot identifier (from Layer 2)
    int currentNodeId;        ///< Current NavMesh node
    int targetNodeId;         ///< Target NavMesh node
    bool isMoving;            ///< Whether robot is actively moving
    
    /**
     * @brief Default constructor.
     */
    RobotObstacle()
        : ObstacleData()
        , robotId(-1)
        , currentNodeId(-1)
        , targetNodeId(-1)
        , isMoving(false) {}
    
    /**
     * @brief Construct from robot state.
     */
    RobotObstacle(int id, 
                  const Backend::Common::Coordinates& pos,
                  const Vector2& vel,
                  double r,
                  int currentNode,
                  int targetNode)
        : ObstacleData(id, pos, vel, r)
        , robotId(id)
        , currentNodeId(currentNode)
        , targetNodeId(targetNode)
        , isMoving(vel.MagnitudeSquared() > 1e-6) {}
};

} // namespace Physics
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_PHYSICS_OBSTACLEDATA_HH
