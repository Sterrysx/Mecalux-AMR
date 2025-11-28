/**
 * @file main.cc
 * @brief Layer 3 Integration Test
 * 
 * Tests the complete Layer 3 pipeline:
 * 1. Load Layer 1 infrastructure (map, NavMesh)
 * 2. Initialize PathfindingService
 * 3. Create RobotDriver(s)
 * 4. Set goal and run simulation
 * 5. Verify movement towards goal
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

// Layer 1 includes
#include "StaticBitMap.hh"
#include "InflatedBitMap.hh"
#include "NavMesh.hh"
#include "NavMeshGenerator.hh"

// Layer 3 includes
#include "Vector2.hh"
#include "Pathfinding/ThetaStarSolver.hh"
#include "Pathfinding/PathfindingService.hh"
#include "Physics/ObstacleData.hh"
#include "Physics/ORCASolver.hh"
#include "Core/RobotDriver.hh"
#include "Core/FastLoopManager.hh"

// Common includes
#include "Coordinates.hh"
#include "Resolution.hh"

using namespace Backend::Layer1;
using namespace Backend::Layer3;
using namespace Backend::Common;

// =============================================================================
// CONFIGURATION
// =============================================================================

const Resolution MAP_RESOLUTION = Resolution::DECIMETERS;
const float ROBOT_RADIUS_METERS = 0.3f;
const int SIMULATION_TICKS = 50;
const float TICK_DURATION_MS = 50.0f;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void PrintSeparator(char c = '=', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

void PrintHeader(const std::string& title) {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "  " << title << "\n";
    PrintSeparator();
}

void PrintRobotStatus(const Core::RobotDriver& robot, int tick) {
    const auto& pos = robot.GetPosition();
    const auto& vel = robot.GetVelocity();
    
    std::cout << "  Tick " << std::setw(3) << tick 
              << " │ Pos: (" << std::setw(7) << std::fixed << std::setprecision(1) 
              << pos.x << ", " << std::setw(7) << pos.y << ")"
              << " │ Vel: (" << std::setw(6) << std::setprecision(2) 
              << vel.x << ", " << std::setw(6) << vel.y << ")"
              << " │ Speed: " << std::setw(5) << std::setprecision(1) << robot.GetSpeed()
              << " │ State: " << robot.GetStateString()
              << "\n";
}

// =============================================================================
// MAIN
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║               LAYER 3 - ROBOT DRIVER INTEGRATION TEST                     ║\n";
    std::cout << "║               Theta* Pathfinding + ORCA Collision Avoidance               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    
    // =========================================================================
    // PHASE 1: Load Layer 1 Infrastructure
    // =========================================================================
    
    PrintHeader("PHASE 1: Loading Infrastructure");
    
    std::cout << "[INFO] Loading map...\n";
    
    std::string mapPath = "../layer1/assets/map_layout.txt";
    StaticBitMap staticMap = StaticBitMap::CreateFromFile(mapPath, MAP_RESOLUTION);
    auto [mapWidth, mapHeight] = staticMap.GetDimensions();
    std::cout << "[INFO] Map: " << mapWidth << "x" << mapHeight << " pixels\n";
    
    std::cout << "[INFO] Creating inflated map (robot radius: " 
              << ROBOT_RADIUS_METERS << "m)...\n";
    InflatedBitMap inflatedMap(staticMap, ROBOT_RADIUS_METERS);
    
    std::cout << "[INFO] Generating NavMesh...\n";
    NavMesh navMesh;
    NavMeshGenerator generator;
    generator.ComputeRecast(inflatedMap, navMesh);
    std::cout << "[INFO] NavMesh: " << navMesh.GetAllNodes().size() << " nodes\n";
    
    // =========================================================================
    // PHASE 2: Initialize PathfindingService
    // =========================================================================
    
    PrintHeader("PHASE 2: Initializing Services");
    
    auto& pathService = Pathfinding::PathfindingService::GetInstance();
    pathService.Initialize(inflatedMap);
    std::cout << "[INFO] PathfindingService initialized\n";
    
    // =========================================================================
    // PHASE 3: Create Robot and Set Goal
    // =========================================================================
    
    PrintHeader("PHASE 3: Creating Robot");
    
    // Find a start and end node
    const auto& nodes = navMesh.GetAllNodes();
    if (nodes.size() < 2) {
        std::cerr << "[ERROR] Not enough nodes in NavMesh!\n";
        return 1;
    }
    
    // Pick start node (index 0) and end node (far from start)
    // Node ID is the index in the vector
    size_t startNodeIdx = 0;
    size_t endNodeIdx = nodes.size() - 1;
    
    // Find a node that is far from the start
    const auto& startNode = nodes[startNodeIdx];
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        if (node.coords.x > static_cast<int>(mapWidth * 0.7) && 
            node.coords.y > static_cast<int>(mapHeight * 0.5)) {
            endNodeIdx = i;
            break;
        }
    }
    
    Coordinates startPos = startNode.coords;
    
    std::cout << "[INFO] Start position: (" << startPos.x << ", " << startPos.y << ")\n";
    
    // Get end node coordinates
    Coordinates endPos = nodes[endNodeIdx].coords;
    std::cout << "[INFO] Target position: (" << endPos.x << ", " << endPos.y << ")\n";
    
    // Calculate distance
    double dx = endPos.x - startPos.x;
    double dy = endPos.y - startPos.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    std::cout << "[INFO] Distance: " << distance << " pixels (" 
              << distance * GetConversionFactorToMeters(MAP_RESOLUTION) << " meters)\n";
    
    // Create robot driver
    Core::RobotDriver driver(0, startPos, navMesh);
    
    // Configure driver
    Core::DriverConfig config;
    config.maxSpeed = 16.0;           // 1.6 m/s at DECIMETERS
    config.acceleration = 8.0;        // 0.8 m/s²
    config.waypointThreshold = 5.0;   // 0.5m
    config.goalThreshold = 5.0;       // 0.5m
    config.robotRadius = 3.0;         // 0.3m
    driver.SetConfig(config);
    
    std::cout << "[INFO] Robot 0 created\n";
    
    // Set goal using node index as ID
    std::cout << "[INFO] Setting goal to node " << endNodeIdx << "...\n";
    if (!driver.SetGoal(static_cast<int>(endNodeIdx))) {
        std::cerr << "[ERROR] Failed to set goal!\n";
        return 1;
    }
    
    // Print path info
    const auto& path = driver.GetPath();
    std::cout << "[INFO] Path waypoints: " << path.size() << "\n";
    std::cout << "[INFO] ETA: " << driver.GetETA() << " seconds\n";
    
    // =========================================================================
    // PHASE 4: Run Simulation
    // =========================================================================
    
    PrintHeader("PHASE 4: Running Simulation (" + std::to_string(SIMULATION_TICKS) + " ticks)");
    
    float dt = TICK_DURATION_MS / 1000.0f;
    std::vector<Physics::ObstacleData> noNeighbors;  // Single robot, no neighbors
    
    std::cout << "\n";
    std::cout << "  Tick   │       Position       │       Velocity      │ Speed │    State\n";
    std::cout << "  ───────┼──────────────────────┼─────────────────────┼───────┼─────────────────\n";
    
    int arrivedTick = -1;
    
    for (int tick = 1; tick <= SIMULATION_TICKS; ++tick) {
        // Update robot
        driver.UpdateLoop(dt, noNeighbors);
        
        // Print status every 5 ticks or on state change
        if (tick % 5 == 1 || tick == SIMULATION_TICKS || 
            driver.GetState() == Core::DriverState::ARRIVED) {
            PrintRobotStatus(driver, tick);
        }
        
        // Check if arrived
        if (driver.GetState() == Core::DriverState::ARRIVED && arrivedTick < 0) {
            arrivedTick = tick;
        }
    }
    
    // =========================================================================
    // PHASE 5: Results Summary
    // =========================================================================
    
    PrintHeader("PHASE 5: Results Summary");
    
    const auto& finalPos = driver.GetPosition();
    double finalDx = finalPos.x - endPos.x;
    double finalDy = finalPos.y - endPos.y;
    double finalDist = std::sqrt(finalDx * finalDx + finalDy * finalDy);
    
    std::cout << "[RESULT] Final state: " << driver.GetStateString() << "\n";
    std::cout << "[RESULT] Final position: (" << finalPos.x << ", " << finalPos.y << ")\n";
    std::cout << "[RESULT] Distance to goal: " << finalDist << " pixels\n";
    
    if (arrivedTick > 0) {
        std::cout << "[RESULT] Arrived at tick: " << arrivedTick 
                  << " (" << arrivedTick * dt << " seconds)\n";
        std::cout << "[SUCCESS] ✓ Robot reached goal!\n";
    } else {
        std::cout << "[RESULT] Robot did not reach goal in " << SIMULATION_TICKS << " ticks\n";
        std::cout << "[INFO] Remaining path length: " << driver.GetRemainingPathLength() << " pixels\n";
        std::cout << "[INFO] Remaining ETA: " << driver.GetETA() << " seconds\n";
    }
    
    // =========================================================================
    // PHASE 6: Multi-Robot Test (Optional)
    // =========================================================================
    
    PrintHeader("PHASE 6: Multi-Robot Collision Avoidance Test");
    
    // Create a FastLoopManager with multiple robots
    Core::FastLoopManager manager(TICK_DURATION_MS);
    
    // Create two robots heading towards each other
    // Robot 0: left to right
    Coordinates robot0Start{50, mapHeight / 2};
    Coordinates robot0End{mapWidth - 50, mapHeight / 2};
    
    // Robot 1: right to left (collision course)
    Coordinates robot1Start{mapWidth - 50, mapHeight / 2 + 10};
    Coordinates robot1End{50, mapHeight / 2 + 10};
    
    // Create robots first (don't store references as vector may reallocate)
    manager.CreateRobot(0, robot0Start, navMesh);
    manager.CreateRobot(1, robot1Start, navMesh);
    
    // Now set goals using index-based access
    manager.GetRobotByIndex(0).SetGoalPosition(robot0End);
    manager.GetRobotByIndex(1).SetGoalPosition(robot1End);
    
    std::cout << "[INFO] Created 2 robots on collision course\n";
    std::cout << "[INFO] Robot 0: (" << robot0Start.x << "," << robot0Start.y 
              << ") → (" << robot0End.x << "," << robot0End.y << ")\n";
    std::cout << "[INFO] Robot 1: (" << robot1Start.x << "," << robot1Start.y 
              << ") → (" << robot1End.x << "," << robot1End.y << ")\n";
    
    // Run simulation with tick callback
    int collisionEvents = 0;
    manager.SetOnTick([&](int tick, [[maybe_unused]] float tickDt) {
        // Check for near-collisions
        const auto& r0 = manager.GetRobotByIndex(0);
        const auto& r1 = manager.GetRobotByIndex(1);
        
        double distBetween = std::sqrt(
            std::pow(r0.GetPosition().x - r1.GetPosition().x, 2.0) +
            std::pow(r0.GetPosition().y - r1.GetPosition().y, 2.0)
        );
        
        double combinedRadius = r0.GetRadius() + r1.GetRadius();
        
        if (distBetween < combinedRadius + 10) {  // Within safety margin
            if (r0.GetState() == Core::DriverState::COLLISION_WAIT ||
                r1.GetState() == Core::DriverState::COLLISION_WAIT) {
                collisionEvents++;
            }
        }
        
        // Print every 10 ticks
        if (tick % 10 == 0) {
            std::cout << "  Tick " << std::setw(3) << tick 
                      << " │ Distance: " << std::setw(6) << std::fixed 
                      << std::setprecision(1) << distBetween << " px\n";
        }
    });
    
    std::cout << "\n  Running 100 ticks...\n\n";
    manager.RunTicks(100);
    
    std::cout << "\n[RESULT] Collision avoidance events: " << collisionEvents << "\n";
    
    // Print final states
    manager.PrintRobotStates();
    
    // Print statistics
    const auto& stats = manager.GetStats();
    std::cout << "\n[STATS] Total ticks: " << stats.tickCount << "\n";
    std::cout << "[STATS] Avg tick time: " << std::setprecision(3) << stats.avgTickTimeMs << " ms\n";
    std::cout << "[STATS] Max tick time: " << stats.maxTickTimeMs << " ms\n";
    std::cout << "[STATS] Simulation time: " << manager.GetSimulationTime() << " seconds\n";
    
    // =========================================================================
    // CLEANUP
    // =========================================================================
    
    PrintHeader("TEST COMPLETE");
    
    Pathfinding::PathfindingService::DestroyInstance();
    
    std::cout << "[INFO] Layer 3 integration test completed successfully!\n\n";
    
    return 0;
}
