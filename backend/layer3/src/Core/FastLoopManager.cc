/**
 * @file FastLoopManager.cc
 * @brief Implementation of the fast physics loop manager
 */

#include "Core/FastLoopManager.hh"
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace Backend {
namespace Layer3 {
namespace Core {

// =============================================================================
// CONSTRUCTORS
// =============================================================================

FastLoopManager::FastLoopManager()
    : tickDuration_(0.05f)        // 50ms default
    , neighborRadius_(100.0)      // 10m at DECIMETERS
    , isRunning_(false)
    , simulationTime_(0.0) {}

FastLoopManager::FastLoopManager(float tickDurationMs)
    : tickDuration_(tickDurationMs / 1000.0f)
    , neighborRadius_(100.0)
    , isRunning_(false)
    , simulationTime_(0.0) {}

// =============================================================================
// ROBOT MANAGEMENT
// =============================================================================

int FastLoopManager::AddRobot(const RobotDriver& robot) {
    robots_.push_back(robot);
    return static_cast<int>(robots_.size() - 1);
}

int FastLoopManager::AddRobot(RobotDriver&& robot) {
    robots_.push_back(std::move(robot));
    return static_cast<int>(robots_.size() - 1);
}

RobotDriver& FastLoopManager::CreateRobot(
    int id,
    const Backend::Common::Coordinates& position,
    const Backend::Layer1::NavMesh& navMesh
) {
    robots_.emplace_back(id, position, navMesh);
    return robots_.back();
}

RobotDriver* FastLoopManager::GetRobot(int robotId) {
    for (auto& robot : robots_) {
        if (robot.GetRobotId() == robotId) {
            return &robot;
        }
    }
    return nullptr;
}

RobotDriver& FastLoopManager::GetRobotByIndex(size_t index) {
    return robots_.at(index);
}

bool FastLoopManager::RemoveRobot(int robotId) {
    auto it = std::remove_if(robots_.begin(), robots_.end(),
        [robotId](const RobotDriver& r) { return r.GetRobotId() == robotId; });
    
    if (it != robots_.end()) {
        robots_.erase(it, robots_.end());
        return true;
    }
    return false;
}

void FastLoopManager::ClearRobots() {
    robots_.clear();
}

// =============================================================================
// SIMULATION CONTROL
// =============================================================================

int FastLoopManager::RunForDuration(float durationSeconds) {
    int numTicks = static_cast<int>(durationSeconds / tickDuration_);
    RunTicks(numTicks);
    return numTicks;
}

void FastLoopManager::RunTicks(int numTicks) {
    isRunning_ = true;
    
    for (int i = 0; i < numTicks && isRunning_; ++i) {
        ExecuteTick();
    }
    
    isRunning_ = false;
}

void FastLoopManager::RunSingleTick() {
    ExecuteTick();
}

void FastLoopManager::Stop() {
    isRunning_ = false;
}

// =============================================================================
// CONFIGURATION
// =============================================================================

void FastLoopManager::SetTickDuration(float durationMs) {
    tickDuration_ = durationMs / 1000.0f;
}

// =============================================================================
// STATISTICS
// =============================================================================

void FastLoopManager::ResetStats() {
    stats_ = LoopStats();
    simulationTime_ = 0.0;
}

// =============================================================================
// DEBUG
// =============================================================================

void FastLoopManager::PrintRobotStates() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ROBOT STATES                               ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    
    for (const auto& robot : robots_) {
        const auto& pos = robot.GetPosition();
        
        std::cout << "║ Robot " << std::setw(2) << robot.GetRobotId() 
                  << " │ State: " << std::setw(14) << robot.GetStateString()
                  << " │ Pos: (" << std::setw(6) << std::fixed << std::setprecision(1) 
                  << pos.x << ", " << std::setw(6) << pos.y << ")"
                  << " │ Vel: " << std::setw(5) << std::setprecision(1) << robot.GetSpeed()
                  << " ║\n";
    }
    
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
}

// =============================================================================
// INTERNAL METHODS
// =============================================================================

std::vector<Physics::ObstacleData> FastLoopManager::GatherNeighbors(size_t robotIndex) const {
    std::vector<Physics::ObstacleData> neighbors;
    
    if (robotIndex >= robots_.size()) {
        return neighbors;
    }
    
    const auto& me = robots_[robotIndex];
    const auto& myPos = me.GetPosition();
    
    for (size_t i = 0; i < robots_.size(); ++i) {
        if (i == robotIndex) continue;
        
        const auto& other = robots_[i];
        const auto& otherPos = other.GetPosition();
        
        // Calculate distance
        double dx = otherPos.x - myPos.x;
        double dy = otherPos.y - myPos.y;
        double dist = std::sqrt(dx * dx + dy * dy);
        
        // Only include neighbors within radius
        if (dist <= neighborRadius_) {
            neighbors.push_back(other.GetObstacleData());
        }
    }
    
    return neighbors;
}

void FastLoopManager::ExecuteTick() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Update all robots
    for (size_t i = 0; i < robots_.size(); ++i) {
        auto neighbors = GatherNeighbors(i);
        robots_[i].UpdateLoop(tickDuration_, neighbors);
    }
    
    // Update simulation time
    simulationTime_ += tickDuration_;
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double tickTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    stats_.tickCount++;
    stats_.totalTimeMs += tickTimeMs;
    stats_.avgTickTimeMs = stats_.totalTimeMs / stats_.tickCount;
    stats_.maxTickTimeMs = std::max(stats_.maxTickTimeMs, tickTimeMs);
    
    // Call tick callback
    if (onTick_) {
        onTick_(stats_.tickCount, tickDuration_);
    }
}

} // namespace Core
} // namespace Layer3
} // namespace Backend
