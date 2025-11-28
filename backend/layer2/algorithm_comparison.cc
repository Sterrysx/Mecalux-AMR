/**
 * @file algorithm_comparison.cc
 * @brief Compare VRP solver algorithms: Hill Climbing, Simulated Annealing, Tabu Search
 * 
 * This benchmark runs all three algorithms on the same problem and compares:
 * - Computation time (ms)
 * - Solution quality (makespan in pixels)
 * - Estimated operating time (seconds)
 * - Trade-off analysis (more compute time vs better solution)
 * 
 * Usage:
 *   make compare
 *   ./build/algorithm_comparison
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>

// Layer 1 includes
#include "../layer1/include/StaticBitMap.hh"
#include "../layer1/include/InflatedBitMap.hh"
#include "../layer1/include/NavMesh.hh"
#include "../layer1/include/NavMeshGenerator.hh"
#include "../layer1/include/POIRegistry.hh"

// Layer 2 includes
#include "include/Task.hh"
#include "include/TaskLoader.hh"
#include "include/RobotAgent.hh"
#include "include/CostMatrixProvider.hh"
#include "include/HillClimbing.hh"
#include "include/SimulatedAnnealing.hh"
#include "include/TabuSearch.hh"

// Common includes
#include "../common/include/Coordinates.hh"
#include "../common/include/Resolution.hh"

using namespace Backend::Layer1;
using namespace Backend::Layer2;
using namespace Backend::Common;

// =============================================================================
// CONFIGURATION (use Layer2 constants directly)
// =============================================================================

const Resolution MAP_RESOLUTION = Resolution::DECIMETERS;
const float ROBOT_SPEED_MPS = 1.6f;

// =============================================================================
// RESULT STRUCTURE
// =============================================================================

struct AlgorithmResult {
    std::string name;
    double computationTimeMs;
    double makespanPixels;
    double makespanSeconds;
    double totalDistancePixels;
    int numRobots;
    int numTasks;
    bool success;
};

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

double PixelsToSeconds(double pixels, Resolution res) {
    double metersPerPixel = GetConversionFactorToMeters(res);
    double meters = pixels * metersPerPixel;
    return meters / ROBOT_SPEED_MPS;
}

void PrintSeparator(char c = '=', int width = 80) {
    std::cout << std::string(width, c) << "\n";
}

void PrintHeader(const std::string& title) {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "  " << title << "\n";
    PrintSeparator();
}

// =============================================================================
// SIMULATION FUNCTION (simplified)
// =============================================================================

double SimulateWithBattery(
    const VRPResult& solution,
    const std::vector<RobotAgent>& originalRobots,
    const CostMatrixProvider& costs
) {
    double maxTime = 0.0;
    
    for (const auto& [robotId, itinerary] : solution.robotItineraries) {
        if (itinerary.empty()) continue;
        
        // Find robot's starting position
        int startNode = -1;
        for (const auto& robot : originalRobots) {
            if (robot.GetRobotId() == robotId) {
                startNode = robot.GetCurrentNodeId();
                break;
            }
        }
        if (startNode < 0) continue;
        
        double time = 0.0;
        double battery = Backend::Layer2::BATTERY_FULL_SECONDS;
        int currentNode = startNode;
        
        for (int targetNode : itinerary) {
            float travelCost = costs.GetCost(currentNode, targetNode);
            double travelTime = PixelsToSeconds(travelCost, MAP_RESOLUTION);
            
            // Check if we need to charge
            double batteryNeeded = travelTime;
            if (battery - batteryNeeded < Backend::Layer2::BATTERY_LOW_THRESHOLD * Backend::Layer2::BATTERY_FULL_SECONDS) {
                // Need to charge
                double chargeNeeded = Backend::Layer2::BATTERY_FULL_SECONDS - battery;
                double chargeTime = (chargeNeeded / Backend::Layer2::BATTERY_FULL_SECONDS) * Backend::Layer2::BATTERY_CHARGE_TIME;
                time += chargeTime;
                battery = Backend::Layer2::BATTERY_FULL_SECONDS;
            }
            
            time += travelTime;
            battery -= batteryNeeded;
            currentNode = targetNode;
        }
        
        maxTime = std::max(maxTime, time);
    }
    
    return maxTime;
}

// =============================================================================
// MAIN
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         VRP ALGORITHM COMPARISON BENCHMARK                                ║\n";
    std::cout << "║         Hill Climbing vs Simulated Annealing vs Tabu Search               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    
    // =========================================================================
    // PHASE 1: Load Infrastructure
    // =========================================================================
    
    PrintHeader("PHASE 1: Loading Infrastructure");
    
    std::cout << "[INFO] Loading map and NavMesh...\n";
    
    std::string mapPath = "../layer1/assets/map_layout.txt";
    StaticBitMap staticMap = StaticBitMap::CreateFromFile(mapPath, MAP_RESOLUTION);
    auto [mapWidth, mapHeight] = staticMap.GetDimensions();
    std::cout << "[INFO] Map: " << mapWidth << "x" << mapHeight << " pixels\n";
    
    const float ROBOT_RADIUS_METERS = 0.3f;
    InflatedBitMap inflatedMap(staticMap, ROBOT_RADIUS_METERS);
    
    NavMesh navMesh;
    NavMeshGenerator generator;
    generator.ComputeRecast(inflatedMap, navMesh);
    std::cout << "[INFO] NavMesh: " << navMesh.GetAllNodes().size() << " nodes\n";
    
    // Load POIs
    POIRegistry poiRegistry;
    poiRegistry.LoadFromJSON("../layer1/assets/poi_config.json");
    poiRegistry.ValidateAndMapToNavMesh(navMesh, inflatedMap);
    
    auto chargingStations = poiRegistry.GetPOIsByType(POIType::CHARGING);
    std::cout << "[INFO] Charging stations: " << chargingStations.size() << "\n";
    
    // =========================================================================
    // PHASE 2: Load Tasks and Create Robots
    // =========================================================================
    
    PrintHeader("PHASE 2: Loading Tasks and Creating Robots");
    
    // Create robots at charging stations
    std::vector<RobotAgent> robots;
    int numRobots = std::min(static_cast<int>(chargingStations.size()), 6);
    
    for (int i = 0; i < numRobots; ++i) {
        int nodeId = poiRegistry.GetNodeForPOI(chargingStations[i]->id);
        // RobotAgent(id, batteryCapacity, chargingNode, speed, loadCapacity)
        RobotAgent robot(i, Backend::Layer2::BATTERY_FULL_SECONDS, nodeId, ROBOT_SPEED_MPS, 1);
        robot.GetMutableState().currentNodeId = nodeId;
        robot.GetMutableState().currentBatteryLevel = Backend::Layer2::BATTERY_FULL_SECONDS;
        robots.push_back(robot);
    }
    std::cout << "[INFO] Created " << robots.size() << " robots\n";
    
    // Load tasks
    std::string taskPath = "../../api/set_of_tasks.json";
    auto tasks = TaskLoader::LoadTasksWithPOI(taskPath, navMesh, poiRegistry);
    std::cout << "[INFO] Loaded " << tasks.size() << " tasks\n";
    
    // =========================================================================
    // PHASE 3: Precompute Cost Matrix
    // =========================================================================
    
    PrintHeader("PHASE 3: Precomputing Cost Matrix");
    
    // Collect all POI nodes
    std::vector<int> allNodes;
    
    // Add charging station nodes
    for (const auto* poi : chargingStations) {
        if (poi->isActive) {
            int nodeId = poiRegistry.GetNodeForPOI(poi->id);
            if (nodeId >= 0) allNodes.push_back(nodeId);
        }
    }
    
    // Add pickup nodes
    auto pickupPOIs = poiRegistry.GetPOIsByType(POIType::PICKUP);
    for (const auto* poi : pickupPOIs) {
        if (poi->isActive) {
            int nodeId = poiRegistry.GetNodeForPOI(poi->id);
            if (nodeId >= 0) allNodes.push_back(nodeId);
        }
    }
    
    // Add dropoff nodes
    auto dropoffPOIs = poiRegistry.GetPOIsByType(POIType::DROPOFF);
    for (const auto* poi : dropoffPOIs) {
        if (poi->isActive) {
            int nodeId = poiRegistry.GetNodeForPOI(poi->id);
            if (nodeId >= 0) allNodes.push_back(nodeId);
        }
    }
    
    // Remove duplicates
    std::sort(allNodes.begin(), allNodes.end());
    allNodes.erase(std::unique(allNodes.begin(), allNodes.end()), allNodes.end());
    
    CostMatrixProvider costMatrix(navMesh);
    costMatrix.PrecomputeForNodes(allNodes);
    std::cout << "[INFO] Cost matrix: " << allNodes.size() << " nodes\n";
    
    // =========================================================================
    // PHASE 4: Run Algorithms
    // =========================================================================
    
    PrintHeader("PHASE 4: Running Algorithms");
    
    std::vector<AlgorithmResult> results;
    
    // --- Hill Climbing ---
    {
        std::cout << "\n--- Running Hill Climbing ---\n";
        std::vector<RobotAgent> robotsCopy = robots;
        
        HillClimbing hc(50, 5);  // Faster settings for comparison
        auto solution = hc.Solve(tasks, robotsCopy, costMatrix);
        
        AlgorithmResult result;
        result.name = "Hill Climbing";
        result.computationTimeMs = solution.computationTimeMs;
        result.makespanPixels = solution.makespan;
        result.makespanSeconds = PixelsToSeconds(solution.makespan, MAP_RESOLUTION);
        result.totalDistancePixels = solution.totalDistance;
        result.numRobots = numRobots;
        result.numTasks = static_cast<int>(tasks.size());
        result.success = solution.isFeasible;
        
        // Simulate with battery
        double simTime = SimulateWithBattery(solution, robots, costMatrix);
        result.makespanSeconds = simTime;
        
        results.push_back(result);
    }
    
    // --- Simulated Annealing ---
    {
        std::cout << "\n--- Running Simulated Annealing ---\n";
        std::vector<RobotAgent> robotsCopy = robots;
        
        SimulatedAnnealing sa(1000.0, 0.95, 1.0, 30);
        auto solution = sa.Solve(tasks, robotsCopy, costMatrix);
        
        AlgorithmResult result;
        result.name = "Simulated Annealing";
        result.computationTimeMs = solution.computationTimeMs;
        result.makespanPixels = solution.makespan;
        result.makespanSeconds = PixelsToSeconds(solution.makespan, MAP_RESOLUTION);
        result.totalDistancePixels = solution.totalDistance;
        result.numRobots = numRobots;
        result.numTasks = static_cast<int>(tasks.size());
        result.success = solution.isFeasible;
        
        double simTime = SimulateWithBattery(solution, robots, costMatrix);
        result.makespanSeconds = simTime;
        
        results.push_back(result);
    }
    
    // --- Tabu Search ---
    {
        std::cout << "\n--- Running Tabu Search ---\n";
        std::vector<RobotAgent> robotsCopy = robots;
        
        TabuSearch ts(80, 15, 25);
        auto solution = ts.Solve(tasks, robotsCopy, costMatrix);
        
        AlgorithmResult result;
        result.name = "Tabu Search";
        result.computationTimeMs = solution.computationTimeMs;
        result.makespanPixels = solution.makespan;
        result.makespanSeconds = PixelsToSeconds(solution.makespan, MAP_RESOLUTION);
        result.totalDistancePixels = solution.totalDistance;
        result.numRobots = numRobots;
        result.numTasks = static_cast<int>(tasks.size());
        result.success = solution.isFeasible;
        
        double simTime = SimulateWithBattery(solution, robots, costMatrix);
        result.makespanSeconds = simTime;
        
        results.push_back(result);
    }
    
    // =========================================================================
    // PHASE 5: Results Analysis
    // =========================================================================
    
    PrintHeader("PHASE 5: Results Analysis");
    
    // Find best results
    double bestMakespan = results[0].makespanPixels;
    double bestTime = results[0].computationTimeMs;
    std::string bestMakespanAlgo = results[0].name;
    std::string fastestAlgo = results[0].name;
    
    for (const auto& r : results) {
        if (r.makespanPixels < bestMakespan) {
            bestMakespan = r.makespanPixels;
            bestMakespanAlgo = r.name;
        }
        if (r.computationTimeMs < bestTime) {
            bestTime = r.computationTimeMs;
            fastestAlgo = r.name;
        }
    }
    
    // Print comparison table
    std::cout << "\n";
    std::cout << "┌────────────────────────┬──────────────┬──────────────┬──────────────┬──────────────┐\n";
    std::cout << "│ Algorithm              │ Compute (ms) │ Makespan(px) │ Operate (s)  │ Distance(px) │\n";
    std::cout << "├────────────────────────┼──────────────┼──────────────┼──────────────┼──────────────┤\n";
    
    for (const auto& r : results) {
        std::cout << "│ " << std::left << std::setw(22) << r.name 
                  << " │ " << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.computationTimeMs
                  << " │ " << std::setw(12) << std::setprecision(0) << r.makespanPixels
                  << " │ " << std::setw(12) << std::setprecision(1) << r.makespanSeconds
                  << " │ " << std::setw(12) << std::setprecision(0) << r.totalDistancePixels
                  << " │\n";
    }
    
    std::cout << "└────────────────────────┴──────────────┴──────────────┴──────────────┴──────────────┘\n";
    
    // =========================================================================
    // PHASE 6: Relative Comparison
    // =========================================================================
    
    PrintHeader("PHASE 6: Relative Comparison");
    
    std::cout << "\n  Problem: " << results[0].numTasks << " tasks, " << results[0].numRobots << " robots\n\n";
    
    // Use first algorithm (Hill Climbing) as baseline
    const auto& baseline = results[0];
    
    std::cout << "  Baseline: " << baseline.name << "\n\n";
    
    std::cout << "┌────────────────────────┬────────────────────┬────────────────────┬────────────────────┐\n";
    std::cout << "│ Algorithm              │ Compute Time Δ     │ Makespan Δ         │ Trade-off Ratio    │\n";
    std::cout << "├────────────────────────┼────────────────────┼────────────────────┼────────────────────┤\n";
    
    for (const auto& r : results) {
        double computeDelta = ((r.computationTimeMs - baseline.computationTimeMs) / baseline.computationTimeMs) * 100.0;
        double makespanDelta = ((r.makespanPixels - baseline.makespanPixels) / baseline.makespanPixels) * 100.0;
        
        // Trade-off: negative = better (lower makespan), positive = worse
        // If we spend X% more compute time and get Y% better makespan, ratio = Y/X
        double tradeoffRatio = 0.0;
        if (std::abs(computeDelta) > 0.01) {
            tradeoffRatio = -makespanDelta / computeDelta;
        }
        
        std::string computeStr = (computeDelta >= 0 ? "+" : "") + 
                                  std::to_string(static_cast<int>(computeDelta)) + "%";
        std::string makespanStr = (makespanDelta >= 0 ? "+" : "") + 
                                   std::to_string(static_cast<int>(makespanDelta)) + "%";
        
        std::string tradeoffStr;
        if (r.name == baseline.name) {
            tradeoffStr = "baseline";
        } else if (tradeoffRatio > 0) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << tradeoffRatio << " (good)";
            tradeoffStr = oss.str();
        } else if (tradeoffRatio < 0) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << tradeoffRatio << " (poor)";
            tradeoffStr = oss.str();
        } else {
            tradeoffStr = "neutral";
        }
        
        std::cout << "│ " << std::left << std::setw(22) << r.name
                  << " │ " << std::right << std::setw(18) << computeStr
                  << " │ " << std::setw(18) << makespanStr
                  << " │ " << std::setw(18) << tradeoffStr
                  << " │\n";
    }
    
    std::cout << "└────────────────────────┴────────────────────┴────────────────────┴────────────────────┘\n";
    
    // =========================================================================
    // PHASE 7: Summary
    // =========================================================================
    
    PrintHeader("SUMMARY");
    
    std::cout << "\n";
    std::cout << "  🏆 BEST MAKESPAN:      " << bestMakespanAlgo << " (" << std::fixed << std::setprecision(0) << bestMakespan << " px)\n";
    std::cout << "  ⚡ FASTEST COMPUTE:    " << fastestAlgo << " (" << std::fixed << std::setprecision(2) << bestTime << " ms)\n";
    std::cout << "\n";
    
    // Time savings analysis
    if (bestMakespanAlgo != fastestAlgo) {
        double timeSavedSeconds = 0;
        double extraComputeMs = 0;
        
        for (const auto& r : results) {
            if (r.name == bestMakespanAlgo) {
                timeSavedSeconds = baseline.makespanSeconds - r.makespanSeconds;
                extraComputeMs = r.computationTimeMs - bestTime;
            }
        }
        
        std::cout << "  ANALYSIS:\n";
        std::cout << "    • Best solution saves " << std::fixed << std::setprecision(1) << timeSavedSeconds << "s of operating time\n";
        std::cout << "    • At the cost of " << std::fixed << std::setprecision(2) << extraComputeMs << "ms extra computation\n";
        
        if (extraComputeMs > 0) {
            double ratio = (timeSavedSeconds * 1000.0) / extraComputeMs;
            std::cout << "    • Trade-off: 1ms compute → " << std::fixed << std::setprecision(1) << ratio << "ms operating time saved\n";
        }
    }
    
    std::cout << "\n";
    PrintSeparator();
    std::cout << "  BENCHMARK COMPLETE\n";
    PrintSeparator();
    std::cout << "\n";
    
    return 0;
}
