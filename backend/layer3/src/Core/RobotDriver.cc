/**
 * @file RobotDriver.cc
 * @brief Implementation of the robot driver
 */

#include "Core/RobotDriver.hh"
#include <iostream>
#include <algorithm>

namespace Backend {
namespace Layer3 {
namespace Core {

// =============================================================================
// CONSTRUCTORS
// =============================================================================

RobotDriver::RobotDriver()
    : robotId_(-1)
    , state_(DriverState::IDLE)
    , precisePosition_(0.0, 0.0)
    , currentPosition_{0, 0}
    , currentVelocity_()
    , currentSpeed_(0.0)
    , pathIndex_(0)
    , currentGoalNodeId_(-1)
    , navMesh_(nullptr) {}

RobotDriver::RobotDriver(
    int id,
    const Backend::Common::Coordinates& startPosition,
    const Backend::Layer1::NavMesh& navMesh,
    const DriverConfig& config
)
    : robotId_(id)
    , config_(config)
    , state_(DriverState::IDLE)
    , precisePosition_(static_cast<double>(startPosition.x), static_cast<double>(startPosition.y))
    , currentPosition_(startPosition)
    , currentVelocity_()
    , currentSpeed_(0.0)
    , pathIndex_(0)
    , currentGoalNodeId_(-1)
    , navMesh_(&navMesh) {}

// =============================================================================
// GOAL SETTING
// =============================================================================

bool RobotDriver::SetGoal(int nodeId) {
    if (!navMesh_) {
        std::cerr << "[RobotDriver " << robotId_ << "] ERROR: NavMesh not set\n";
        return false;
    }
    
    // Look up node coordinates (node ID is the index in the vector)
    const auto& nodes = navMesh_->GetAllNodes();
    
    if (nodeId >= 0 && static_cast<size_t>(nodeId) < nodes.size()) {
        // Get coordinates from the node (coords field contains x,y)
        Backend::Common::Coordinates target = nodes[nodeId].coords;
        return SetGoalPosition(target);
    }
    
    std::cerr << "[RobotDriver " << robotId_ << "] ERROR: Node " << nodeId << " not found\n";
    return false;
}

bool RobotDriver::SetGoalPosition(const Backend::Common::Coordinates& target) {
    auto& pathService = Pathfinding::PathfindingService::GetInstance();
    
    if (!pathService.IsInitialized()) {
        std::cerr << "[RobotDriver " << robotId_ << "] ERROR: PathfindingService not initialized\n";
        return false;
    }
    
    // Clear current path
    currentPath_.clear();
    pathIndex_ = 0;
    
    // Set state to computing
    state_ = DriverState::COMPUTING_PATH;
    
    // Request path (synchronous for simplicity)
    auto result = pathService.ComputePathImmediate(currentPosition_, target);
    
    OnPathReceived(result);
    
    return result.success;
}

void RobotDriver::CancelGoal() {
    currentPath_.clear();
    pathIndex_ = 0;
    currentGoalNodeId_ = -1;
    currentVelocity_ = Vector2::Zero();
    currentSpeed_ = 0.0;
    state_ = DriverState::IDLE;
}

bool RobotDriver::HasGoal() const {
    return state_ == DriverState::MOVING || state_ == DriverState::COMPUTING_PATH;
}

// =============================================================================
// UPDATE LOOP
// =============================================================================

void RobotDriver::UpdateLoop(float dt, const std::vector<Physics::ObstacleData>& neighbors) {
    // Handle different states
    switch (state_) {
        case DriverState::IDLE:
        case DriverState::ARRIVED:
        case DriverState::STUCK:
            // No movement needed
            currentVelocity_ = Vector2::Zero();
            currentSpeed_ = 0.0;
            return;
            
        case DriverState::COMPUTING_PATH:
            // Waiting for path - no movement
            return;
            
        case DriverState::MOVING:
        case DriverState::COLLISION_WAIT:
            // Continue to movement logic
            break;
    }
    
    // Check if goal reached
    if (IsGoalReached()) {
        currentVelocity_ = Vector2::Zero();
        currentSpeed_ = 0.0;
        state_ = DriverState::ARRIVED;
        
        if (onGoalReached_) {
            onGoalReached_(robotId_, currentGoalNodeId_);
        }
        return;
    }
    
    // Check if waypoint reached
    while (pathIndex_ < currentPath_.size() && IsWaypointReached()) {
        AdvanceWaypoint();
    }
    
    // STEP 1: Calculate preferred velocity
    Vector2 preferredVelocity = CalculatePreferredVelocity();
    
    // STEP 2: Apply ORCA collision avoidance
    Physics::ObstacleData myData = GetObstacleData();
    Vector2 safeVelocity = orcaSolver_.CalculateSafeVelocity(myData, neighbors, preferredVelocity);
    
    // STEP 3: Apply acceleration limits
    Vector2 smoothedVelocity = ApplyAcceleration(safeVelocity, dt);
    
    // Update velocity
    currentVelocity_ = smoothedVelocity;
    currentSpeed_ = currentVelocity_.Magnitude();
    
    // Check if we're stuck (ORCA stopped us)
    if (currentSpeed_ < 0.1 && preferredVelocity.Magnitude() > 1.0) {
        state_ = DriverState::COLLISION_WAIT;
    } else if (state_ == DriverState::COLLISION_WAIT && currentSpeed_ > 0.5) {
        state_ = DriverState::MOVING;
    }
    
    // STEP 4: Update position using precise floating-point math
    precisePosition_ = precisePosition_ + currentVelocity_ * static_cast<double>(dt);
    
    // Update integer position for grid lookups
    currentPosition_.x = static_cast<int>(std::round(precisePosition_.x));
    currentPosition_.y = static_cast<int>(std::round(precisePosition_.y));
}

// =============================================================================
// GETTERS
// =============================================================================

Physics::ObstacleData RobotDriver::GetObstacleData() const {
    return Physics::ObstacleData(
        robotId_,
        currentPosition_,
        currentVelocity_,
        config_.robotRadius
    );
}

double RobotDriver::GetRemainingPathLength() const {
    if (pathIndex_ >= currentPath_.size()) {
        return 0.0;
    }
    
    double length = 0.0;
    
    // Distance to next waypoint (use precise position)
    const auto& nextWP = currentPath_[pathIndex_];
    double dx = static_cast<double>(nextWP.x) - precisePosition_.x;
    double dy = static_cast<double>(nextWP.y) - precisePosition_.y;
    length += std::sqrt(dx * dx + dy * dy);
    
    // Sum remaining waypoint-to-waypoint distances
    for (size_t i = pathIndex_; i + 1 < currentPath_.size(); ++i) {
        const auto& a = currentPath_[i];
        const auto& b = currentPath_[i + 1];
        dx = b.x - a.x;
        dy = b.y - a.y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    
    return length;
}

double RobotDriver::GetETA() const {
    if (config_.maxSpeed < 1e-6) return std::numeric_limits<double>::max();
    return GetRemainingPathLength() / config_.maxSpeed;
}

std::string RobotDriver::GetStateString() const {
    switch (state_) {
        case DriverState::IDLE: return "IDLE";
        case DriverState::COMPUTING_PATH: return "COMPUTING_PATH";
        case DriverState::MOVING: return "MOVING";
        case DriverState::ARRIVED: return "ARRIVED";
        case DriverState::STUCK: return "STUCK";
        case DriverState::COLLISION_WAIT: return "COLLISION_WAIT";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// INTERNAL METHODS
// =============================================================================

Vector2 RobotDriver::CalculatePreferredVelocity() const {
    if (pathIndex_ >= currentPath_.size()) {
        return Vector2::Zero();
    }
    
    // Direction to next waypoint (use precise position)
    const auto& target = currentPath_[pathIndex_];
    double dx = static_cast<double>(target.x) - precisePosition_.x;
    double dy = static_cast<double>(target.y) - precisePosition_.y;
    
    Vector2 direction(dx, dy);
    double distance = direction.Magnitude();
    
    if (distance < 1e-6) {
        return Vector2::Zero();
    }
    
    // Normalize and scale to max speed
    direction = direction.Normalized();
    
    // Slow down near goal
    double speed = config_.maxSpeed;
    if (pathIndex_ == currentPath_.size() - 1) {
        // Final waypoint - slow down as we approach
        double slowdownDist = config_.goalThreshold * 3.0;
        if (distance < slowdownDist) {
            speed = config_.maxSpeed * (distance / slowdownDist);
            speed = std::max(speed, 1.0); // Minimum speed
        }
    }
    
    return direction * speed;
}

bool RobotDriver::IsWaypointReached() const {
    if (pathIndex_ >= currentPath_.size()) {
        return false;
    }
    
    const auto& waypoint = currentPath_[pathIndex_];
    double dx = static_cast<double>(waypoint.x) - precisePosition_.x;
    double dy = static_cast<double>(waypoint.y) - precisePosition_.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    
    // Use smaller threshold for intermediate waypoints
    double threshold = (pathIndex_ == currentPath_.size() - 1) 
        ? config_.goalThreshold 
        : config_.waypointThreshold;
    
    return dist < threshold;
}

bool RobotDriver::IsGoalReached() const {
    if (currentPath_.empty()) {
        return false;
    }
    
    const auto& goal = currentPath_.back();
    double dx = static_cast<double>(goal.x) - precisePosition_.x;
    double dy = static_cast<double>(goal.y) - precisePosition_.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    
    return dist < config_.goalThreshold;
}

void RobotDriver::AdvanceWaypoint() {
    if (pathIndex_ < currentPath_.size()) {
        pathIndex_++;
    }
}

void RobotDriver::OnPathReceived(const Pathfinding::PathResult& result) {
    if (!result.success) {
        std::cerr << "[RobotDriver " << robotId_ << "] Path computation failed\n";
        state_ = DriverState::STUCK;
        return;
    }
    
    currentPath_ = result.path;
    pathIndex_ = 0;
    state_ = DriverState::MOVING;
    
    std::cout << "[RobotDriver " << robotId_ << "] Path received: " 
              << currentPath_.size() << " waypoints, " 
              << result.pathLength << " pixels, "
              << result.computeTimeMs << " ms\n";
}

Vector2 RobotDriver::ApplyAcceleration(const Vector2& targetVelocity, float dt) const {
    Vector2 deltaV = targetVelocity - currentVelocity_;
    double deltaMag = deltaV.Magnitude();
    
    // Maximum velocity change this frame
    double maxDelta = config_.acceleration * dt;
    
    if (deltaMag <= maxDelta) {
        return targetVelocity;
    }
    
    // Limit change to max acceleration
    return currentVelocity_ + deltaV.Normalized() * maxDelta;
}

} // namespace Core
} // namespace Layer3
} // namespace Backend
