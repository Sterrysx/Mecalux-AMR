/**
 * @file ORCASolver.cc
 * @brief Implementation of ORCA collision avoidance
 */

#include "Physics/ORCASolver.hh"
#include <algorithm>
#include <iostream>

namespace Backend {
namespace Layer3 {
namespace Physics {

// =============================================================================
// MAIN INTERFACE
// =============================================================================

Vector2 ORCASolver::CalculateSafeVelocity(
    const ObstacleData& me,
    const std::vector<ObstacleData>& neighbors,
    const Vector2& preferredVelocity
) const {
    // If no neighbors, return preferred velocity (clamped)
    if (neighbors.empty()) {
        return ClampSpeed(preferredVelocity);
    }
    
    // Find closest obstacle
    double closestDistance = std::numeric_limits<double>::max();
    double closestCombinedRadius = 0.0;
    int closestIdx = -1;
    
    for (size_t i = 0; i < neighbors.size(); ++i) {
        double dist = me.DistanceTo(neighbors[i]);
        double combinedR = me.CombinedRadius(neighbors[i]) + config_.safetyMargin;
        double clearance = dist - combinedR;
        
        if (clearance < closestDistance) {
            closestDistance = clearance;
            closestCombinedRadius = combinedR;
            closestIdx = static_cast<int>(i);
        }
    }
    
    // PHASE 1: Stop-and-Wait
    // If too close to any obstacle, stop completely
    if (closestDistance < config_.stopDistance) {
        // Emergency stop - too close to obstacle
        return Vector2::Zero();
    }
    
    // PHASE 2: Apply Slowdown
    // If within slowdown zone, reduce speed proportionally
    Vector2 velocity = preferredVelocity;
    
    if (closestDistance < config_.slowdownDistance) {
        velocity = ApplySlowdown(velocity, closestDistance, closestCombinedRadius);
    }
    
    // PHASE 3: Apply Repulsion
    // Push velocity away from nearby obstacles
    velocity = ApplyRepulsion(me, neighbors, velocity);
    
    // PHASE 4: Check for future collisions
    // If velocity would cause collision within time horizon, reduce further
    for (const auto& neighbor : neighbors) {
        if (WillCollide(me, neighbor, velocity, config_.timeHorizon)) {
            // Reduce velocity towards obstacle
            Vector2 toObstacle = neighbor.GetPositionVec() - me.GetPositionVec();
            Vector2 toObstacleDir = toObstacle.Normalized();
            
            // Remove component towards obstacle
            double dotProduct = velocity.Dot(toObstacleDir);
            if (dotProduct > 0) {
                velocity = velocity - toObstacleDir * (dotProduct * config_.responsiveness);
            }
        }
    }
    
    // Clamp to maximum speed
    return ClampSpeed(velocity);
}

// =============================================================================
// COLLISION DETECTION
// =============================================================================

double ORCASolver::FindClosestObstacleDistance(
    const ObstacleData& me,
    const std::vector<ObstacleData>& neighbors
) const {
    double closest = std::numeric_limits<double>::max();
    
    for (const auto& neighbor : neighbors) {
        double dist = me.DistanceTo(neighbor);
        double clearance = dist - me.CombinedRadius(neighbor);
        closest = std::min(closest, clearance);
    }
    
    return closest;
}

bool ORCASolver::WillCollide(
    const ObstacleData& me,
    const ObstacleData& neighbor,
    const Vector2& velocity,
    double timeHorizon
) const {
    // Predict positions after timeHorizon
    Vector2 myPos = me.GetPositionVec();
    Vector2 neighborPos = neighbor.GetPositionVec();
    
    // Relative velocity (assuming neighbor continues at current velocity)
    Vector2 relVel = velocity - neighbor.velocity;
    
    // Relative position
    Vector2 relPos = myPos - neighborPos;
    
    // Check if we're getting closer
    double approachRate = relPos.Normalized().Dot(relVel);
    if (approachRate >= 0) {
        // Moving apart, no collision
        return false;
    }
    
    // Time to closest approach
    double closestT = -relPos.Dot(relVel) / (relVel.MagnitudeSquared() + 1e-10);
    if (closestT < 0 || closestT > timeHorizon) {
        return false;
    }
    
    // Distance at closest approach
    Vector2 closestRelPos = relPos + relVel * closestT;
    double closestDist = closestRelPos.Magnitude();
    
    double combinedRadius = me.CombinedRadius(neighbor) + config_.safetyMargin;
    
    return closestDist < combinedRadius;
}

// =============================================================================
// VELOCITY MODIFICATION
// =============================================================================

Vector2 ORCASolver::ApplyRepulsion(
    const ObstacleData& me,
    const std::vector<ObstacleData>& neighbors,
    const Vector2& velocity
) const {
    Vector2 repulsionForce = Vector2::Zero();
    
    Vector2 myPos = me.GetPositionVec();
    
    for (const auto& neighbor : neighbors) {
        Vector2 neighborPos = neighbor.GetPositionVec();
        Vector2 toMe = myPos - neighborPos;
        
        double dist = toMe.Magnitude();
        double combinedR = me.CombinedRadius(neighbor) + config_.safetyMargin;
        
        // Only apply repulsion within influence range
        double influenceRange = combinedR + config_.slowdownDistance;
        if (dist > influenceRange || dist < 1e-6) {
            continue;
        }
        
        // Repulsion strength inversely proportional to distance
        // Stronger when closer to collision
        double penetration = influenceRange - dist;
        double strength = (penetration / influenceRange) * config_.maxSpeed * 0.5;
        
        Vector2 repulsion = toMe.Normalized() * strength;
        repulsionForce += repulsion;
    }
    
    // Blend repulsion with original velocity
    return velocity + repulsionForce * config_.responsiveness;
}

Vector2 ORCASolver::ApplySlowdown(
    const Vector2& velocity,
    double closestDistance,
    double combinedRadius
) const {
    // Linear interpolation between full speed and stop
    // At slowdownDistance: full speed
    // At stopDistance: zero speed
    
    double range = config_.slowdownDistance - config_.stopDistance;
    double t = (closestDistance - config_.stopDistance) / range;
    t = std::clamp(t, 0.0, 1.0);
    
    return velocity * t;
}

Vector2 ORCASolver::ClampSpeed(const Vector2& velocity) const {
    return velocity.LimitMagnitude(config_.maxSpeed);
}

} // namespace Physics
} // namespace Layer3
} // namespace Backend
