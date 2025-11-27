/**
 * =============================================================================
 * Layer 2 (Fleet Manager) Integration Test Driver
 * =============================================================================
 * 
 * This test validates that Layer 2 components work together:
 * 1. Integration with Layer 1 (NavMesh, POIRegistry)
 * 2. Task Loading from JSON with POI IDs (P1, P2, etc.)
 * 3. Cost Matrix Precomputation (A* on NavMesh)
 * 4. Robot Agent Management with Battery System
 * 5. VRP Solving (Hill Climbing)
 * 6. Fleet Schedule Report with Time Calculations
 * 
 * Battery System:
 *   Full Battery: 300 seconds of operation
 *   Low Threshold: 20% → must go to charging zone
 *   Smart Check: At 27%, if task needs >7% battery → go charge first
 *   Charging Time: 60 seconds (0% to 100%)
 * 
 * Physics:
 *   Robot Speed = 1.6 m/s
 *   Time = (PathCost_pixels / PixelsPerMeter) / 1.6 m/s
 * 
 * =============================================================================
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <chrono>

// Layer 1 includes (dependencies)
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
#include "include/IVRPSolver.hh"
#include "include/HillClimbing.hh"

// Common includes
#include "../common/include/Coordinates.hh"
#include "../common/include/Resolution.hh"

using namespace Backend::Layer1;
using namespace Backend::Layer2;
using namespace Backend::Common;

// =============================================================================
// CONSTANTS
// =============================================================================
constexpr float ROBOT_SPEED_MPS = 1.6f;  // Robot speed in meters per second

// =============================================================================
// ANSI Color Codes for test output
// =============================================================================
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

void PrintHeader(const std::string& title) {
    std::cout << "\n" << COLOR_CYAN 
              << "======================================================\n"
              << "  " << title << "\n"
              << "======================================================"
              << COLOR_RESET << "\n\n";
}

void PrintPass(const std::string& test) {
    std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << test << std::endl;
}

void PrintFail(const std::string& test) {
    std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << test << std::endl;
}

void PrintInfo(const std::string& info) {
    std::cout << COLOR_YELLOW << "[INFO] " << COLOR_RESET << info << std::endl;
}

// =============================================================================
// PHYSICS CONVERSION UTILITIES
// =============================================================================

/**
 * @brief Convert path cost (in pixels) to distance (in meters).
 */
double PixelsToMeters(double costPixels, Resolution resolution) {
    double metersPerPixel = GetConversionFactorToMeters(resolution);
    return costPixels * metersPerPixel;
}

/**
 * @brief Convert distance (meters) to travel time (seconds) at robot speed.
 */
double MetersToSeconds(double distanceMeters, double speedMps = ROBOT_SPEED_MPS) {
    return distanceMeters / speedMps;
}

/**
 * @brief Convert path cost (pixels) directly to travel time (seconds).
 */
double PixelsToSeconds(double costPixels, Resolution resolution, double speedMps = ROBOT_SPEED_MPS) {
    double distanceMeters = PixelsToMeters(costPixels, resolution);
    return MetersToSeconds(distanceMeters, speedMps);
}

/**
 * @brief Format time in seconds as HH:MM:SS or MM:SS.
 */
std::string FormatTime(double seconds) {
    int totalSecs = static_cast<int>(std::round(seconds));
    int hours = totalSecs / 3600;
    int mins = (totalSecs % 3600) / 60;
    int secs = totalSecs % 60;
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << std::setfill('0') << std::setw(2) << hours << ":"
            << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    } else {
        oss << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    }
    return oss.str();
}

// =============================================================================
// FLEET SCHEDULE REPORT WITH BATTERY SIMULATION
// =============================================================================

/**
 * @brief Simulate fleet execution with battery management.
 * 
 * This function simulates the execution of all robot itineraries,
 * tracking battery consumption and charging events.
 */
struct SimulationResult {
    double totalOperationTime;      // Total wall-clock time (seconds)
    double totalTravelTime;         // Sum of all travel times
    double totalChargingTime;       // Sum of all charging times
    int totalChargingEvents;        // Number of times robots went to charge
    std::vector<std::string> eventLog;  // Detailed event log
};

SimulationResult SimulateFleetExecution(
    std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costMatrix,
    Resolution resolution,
    const POIRegistry& poiRegistry
) {
    SimulationResult result;
    result.totalOperationTime = 0.0;
    result.totalTravelTime = 0.0;
    result.totalChargingTime = 0.0;
    result.totalChargingEvents = 0;
    
    // Get charging nodes for return trips
    auto chargingNodes = poiRegistry.GetNodesByType(POIType::CHARGING);
    
    std::cout << "\n";
    std::cout << COLOR_BOLD << COLOR_MAGENTA;
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FLEET EXECUTION SIMULATION                            ║\n";
    std::cout << "║           (with Battery Management: 300s full, 60s charge)               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET << "\n";
    
    // Track max robot time (makespan)
    double maxRobotTime = 0.0;
    
    // Simulate each robot
    for (auto& robot : robots) {
        double robotTime = 0.0;
        double robotTravelTime = 0.0;
        double robotChargeTime = 0.0;
        int robotChargeEvents = 0;
        
        std::cout << COLOR_BOLD << "  ┌─────────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "  │ Robot " << robot.GetRobotId() << " Simulation\n";
        std::cout << "  └─────────────────────────────────────────────────────────────────────────┘" << COLOR_RESET << "\n";
        
        const auto& itinerary = robot.GetItinerary();
        if (itinerary.empty()) {
            std::cout << "    Status: " << COLOR_GREEN << "IDLE" << COLOR_RESET << " (no tasks)\n\n";
            continue;
        }
        
        std::cout << "    Starting battery: " << static_cast<int>(robot.GetCurrentBattery() * 100) 
                  << "% (" << std::fixed << std::setprecision(1) << robot.GetBatterySeconds() << "s)\n";
        std::cout << "    Itinerary: " << robot.GetCurrentNodeId();
        for (int node : itinerary) std::cout << " → " << node;
        std::cout << "\n\n";
        
        std::cout << "    " << std::setw(8) << "Time(s)" 
                  << std::setw(12) << "Event"
                  << std::setw(15) << "Battery(%)"
                  << std::setw(20) << "Details" << "\n";
        std::cout << "    " << std::string(55, '-') << "\n";
        
        int currentNode = robot.GetCurrentNodeId();
        int chargerNode = robot.GetChargingStationNode();
        
        for (size_t i = 0; i < itinerary.size(); ++i) {
            int nextNode = itinerary[i];
            
            // Calculate travel time to next node
            float costPx = costMatrix.GetCost(currentNode, nextNode);
            double travelTimeS = PixelsToSeconds(costPx, resolution);
            
            // Check if we need to charge before this leg
            // Also calculate time to return to charger after this task for safety check
            float costToChargerPx = costMatrix.GetCost(nextNode, chargerNode);
            double timeToChargerS = PixelsToSeconds(costToChargerPx, resolution);
            
            if (robot.ShouldChargeBeforeTask(travelTimeS + timeToChargerS)) {
                // Need to go charge first
                float costToChargePx = costMatrix.GetCost(currentNode, chargerNode);
                double travelToChargeS = PixelsToSeconds(costToChargePx, resolution);
                
                // Travel to charger
                robot.ConsumeBattery(static_cast<float>(travelToChargeS));
                robotTime += travelToChargeS;
                robotTravelTime += travelToChargeS;
                
                std::cout << "    " << std::setw(8) << std::fixed << std::setprecision(1) << robotTime
                          << std::setw(12) << "→ CHARGE"
                          << std::setw(15) << static_cast<int>(robot.GetCurrentBattery() * 100)
                          << "    " << "Need charge before task\n";
                
                result.eventLog.push_back("Robot " + std::to_string(robot.GetRobotId()) + 
                                          " went to charge at t=" + std::to_string(robotTime));
                
                // Charge to full
                double chargeTimeS = robot.GetTimeToFullCharge();
                robot.ChargeBattery(static_cast<float>(chargeTimeS));
                robotTime += chargeTimeS;
                robotChargeTime += chargeTimeS;
                robotChargeEvents++;
                
                std::cout << "    " << std::setw(8) << std::fixed << std::setprecision(1) << robotTime
                          << std::setw(12) << "CHARGED"
                          << std::setw(15) << static_cast<int>(robot.GetCurrentBattery() * 100)
                          << "    " << "+" << std::fixed << std::setprecision(1) << chargeTimeS << "s\n";
                
                // Now travel from charger to next node
                costPx = costMatrix.GetCost(chargerNode, nextNode);
                travelTimeS = PixelsToSeconds(costPx, resolution);
                currentNode = chargerNode;
            }
            
            // Travel to next node
            robot.ConsumeBattery(static_cast<float>(travelTimeS));
            robotTime += travelTimeS;
            robotTravelTime += travelTimeS;
            currentNode = nextNode;
            
            std::cout << "    " << std::setw(8) << std::fixed << std::setprecision(1) << robotTime
                      << std::setw(12) << ("→ " + std::to_string(nextNode))
                      << std::setw(15) << static_cast<int>(robot.GetCurrentBattery() * 100)
                      << "    " << "Travel " << std::fixed << std::setprecision(1) << travelTimeS << "s\n";
        }
        
        // Final check: should robot return to charger?
        if (robot.NeedsCharging()) {
            float costToChargePx = costMatrix.GetCost(currentNode, chargerNode);
            double travelToChargeS = PixelsToSeconds(costToChargePx, resolution);
            
            robot.ConsumeBattery(static_cast<float>(travelToChargeS));
            robotTime += travelToChargeS;
            robotTravelTime += travelToChargeS;
            
            std::cout << "    " << std::setw(8) << std::fixed << std::setprecision(1) << robotTime
                      << std::setw(12) << "→ CHARGE"
                      << std::setw(15) << static_cast<int>(robot.GetCurrentBattery() * 100)
                      << "    " << "Return to charger (low battery)\n";
        }
        
        // Summary for this robot
        std::cout << "    " << std::string(55, '=') << "\n";
        std::cout << "    Robot " << robot.GetRobotId() << " Summary:\n";
        std::cout << "      Total time: " << std::fixed << std::setprecision(1) << robotTime 
                  << "s (" << FormatTime(robotTime) << ")\n";
        std::cout << "      Travel time: " << std::fixed << std::setprecision(1) << robotTravelTime << "s\n";
        std::cout << "      Charge time: " << std::fixed << std::setprecision(1) << robotChargeTime << "s\n";
        std::cout << "      Charge events: " << robotChargeEvents << "\n";
        std::cout << "      Final battery: " << static_cast<int>(robot.GetCurrentBattery() * 100) << "%\n\n";
        
        // Update totals
        result.totalTravelTime += robotTravelTime;
        result.totalChargingTime += robotChargeTime;
        result.totalChargingEvents += robotChargeEvents;
        maxRobotTime = std::max(maxRobotTime, robotTime);
    }
    
    result.totalOperationTime = maxRobotTime;
    
    return result;
}

/**
 * @brief Print the final summary with timing information.
 */
void PrintFinalSummary(
    const SimulationResult& simResult,
    double computationTimeMs,
    int passedTests,
    int totalTests
) {
    std::cout << "\n";
    std::cout << COLOR_BOLD << COLOR_CYAN;
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           FINAL SUMMARY                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET << "\n";
    
    std::cout << "  " << COLOR_BOLD << "EXECUTION TIMES:" << COLOR_RESET << "\n";
    std::cout << "  ├─ Total Operation Time (Makespan): " << COLOR_GREEN 
              << std::fixed << std::setprecision(2) << simResult.totalOperationTime << " seconds"
              << " (" << FormatTime(simResult.totalOperationTime) << ")" << COLOR_RESET << "\n";
    std::cout << "  ├─ Total Travel Time:              " 
              << std::fixed << std::setprecision(2) << simResult.totalTravelTime << " seconds\n";
    std::cout << "  ├─ Total Charging Time:            " 
              << std::fixed << std::setprecision(2) << simResult.totalChargingTime << " seconds\n";
    std::cout << "  └─ Total Charging Events:          " << simResult.totalChargingEvents << "\n";
    
    std::cout << "\n  " << COLOR_BOLD << "COMPUTATION TIME:" << COLOR_RESET << "\n";
    std::cout << "  └─ VRP Solving + Preprocessing:    " << COLOR_MAGENTA
              << std::fixed << std::setprecision(2) << computationTimeMs << " ms" 
              << COLOR_RESET << "\n";
    
    std::cout << "\n  " << COLOR_BOLD << "TEST RESULTS:" << COLOR_RESET << "\n";
    if (passedTests == totalTests) {
        std::cout << "  └─ " << COLOR_GREEN << "ALL " << passedTests << " TESTS PASSED!" << COLOR_RESET << "\n";
    } else {
        std::cout << "  └─ " << COLOR_YELLOW << passedTests << "/" << totalTests << " tests passed" << COLOR_RESET << "\n";
    }
    
    std::cout << "\n";
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================
int main() {
    // Start total timing
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    double computationTimeMs = 0.0;
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     LAYER 2 (FLEET MANAGER) INTEGRATION TEST                  ║\n";
    std::cout << "║     VRP Solver + Battery Management + POI Task Loading        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    int totalTests = 0;
    int passedTests = 0;
    
    // Resolution for this test (matching Layer 1 map)
    const Resolution MAP_RESOLUTION = Resolution::DECIMETERS;

    // =========================================================================
    // PHASE 1: Load Layer 1 Dependencies
    // =========================================================================
    PrintHeader("PHASE 1: Loading Layer 1 Dependencies");
    
    // Print configuration info
    double metersPerPixel = GetConversionFactorToMeters(MAP_RESOLUTION);
    double pixelsPerMeter = 1.0 / metersPerPixel;
    PrintInfo("Map Resolution: " + std::string(GetResolutionName(MAP_RESOLUTION)) + 
              " (" + std::to_string(static_cast<int>(pixelsPerMeter)) + " px/m)");
    PrintInfo("Robot Speed: " + std::to_string(ROBOT_SPEED_MPS) + " m/s");
    PrintInfo("Battery: " + std::to_string(static_cast<int>(BATTERY_FULL_SECONDS)) + "s full, " +
              std::to_string(static_cast<int>(BATTERY_CHARGE_TIME)) + "s charge time");
    PrintInfo("Low Battery Threshold: " + std::to_string(static_cast<int>(BATTERY_LOW_THRESHOLD * 100)) + "%");
    PrintInfo("Smart Charge: At " + std::to_string(static_cast<int>(BATTERY_SMART_THRESHOLD * 100)) + 
              "%, if task needs >" + std::to_string(static_cast<int>(BATTERY_SMART_MARGIN * 100)) + "% → charge");

    // --- 1a. Load Static Map (auto-detect dimensions from file) ---
    std::string mapPath = "../layer1/assets/map_layout.txt";
    StaticBitMap staticMap = StaticBitMap::CreateFromFile(mapPath, MAP_RESOLUTION);
    auto [mapWidth, mapHeight] = staticMap.GetDimensions();
    PrintInfo("Map Dimensions: " + std::to_string(mapWidth) + "x" + std::to_string(mapHeight) + " pixels");
    PrintInfo("Physical Size: " + std::to_string(mapWidth * metersPerPixel) + "m x " + 
              std::to_string(mapHeight * metersPerPixel) + "m");
    PrintPass("StaticBitMap loaded");
    passedTests++;
    totalTests++;

    // --- 1b. Generate Inflated Map ---
    const float ROBOT_RADIUS_METERS = 0.3f;
    InflatedBitMap inflatedMap(staticMap, ROBOT_RADIUS_METERS);
    PrintPass("InflatedBitMap generated (robot radius: " + 
              std::to_string(ROBOT_RADIUS_METERS) + "m)");
    passedTests++;
    totalTests++;

    // --- 1c. Generate NavMesh ---
    NavMesh navMesh;
    NavMeshGenerator generator;
    generator.ComputeRecast(inflatedMap, navMesh);
    
    const auto& nodes = navMesh.GetAllNodes();
    if (nodes.empty()) {
        PrintFail("NavMesh has no nodes - cannot continue");
        return 1;
    }
    PrintPass("NavMesh generated with " + std::to_string(nodes.size()) + " nodes");
    passedTests++;
    totalTests++;

    // --- 1d. Load POI Registry ---
    POIRegistry poiRegistry;
    bool poiLoaded = poiRegistry.LoadFromJSON("../layer1/assets/poi_config.json");
    if (!poiLoaded) {
        PrintFail("POI Registry failed to load - cannot continue with POI tasks");
        return 1;
    } else {
        int mappedPOIs = poiRegistry.ValidateAndMapToNavMesh(navMesh, inflatedMap, 0.0f);
        PrintPass("POI Registry loaded with " + std::to_string(mappedPOIs) + " mapped POIs");
        passedTests++;
    }
    totalTests++;
    
    // Print POI summary
    poiRegistry.PrintSummary();

    // =========================================================================
    // PHASE 2: Cost Matrix Precomputation
    // =========================================================================
    PrintHeader("PHASE 2: Cost Matrix Precomputation (A* Algorithm)");

    auto computeStartTime = std::chrono::high_resolution_clock::now();

    // Collect all POI node IDs for cost matrix
    std::vector<int> poiNodeIds;
    
    auto chargingNodes = poiRegistry.GetNodesByType(POIType::CHARGING);
    poiNodeIds.insert(poiNodeIds.end(), chargingNodes.begin(), chargingNodes.end());
    
    auto pickupNodes = poiRegistry.GetNodesByType(POIType::PICKUP);
    poiNodeIds.insert(poiNodeIds.end(), pickupNodes.begin(), pickupNodes.end());
    
    auto dropoffNodes = poiRegistry.GetNodesByType(POIType::DROPOFF);
    poiNodeIds.insert(poiNodeIds.end(), dropoffNodes.begin(), dropoffNodes.end());

    PrintInfo("POI counts - Charging: " + std::to_string(chargingNodes.size()) +
              ", Pickup: " + std::to_string(pickupNodes.size()) +
              ", Dropoff: " + std::to_string(dropoffNodes.size()));

    // Create and populate cost matrix
    CostMatrixProvider costMatrix(navMesh);
    
    if (!poiNodeIds.empty()) {
        int pairs = costMatrix.PrecomputeForNodes(poiNodeIds);
        PrintPass("Cost matrix precomputed for " + std::to_string(pairs) + " node pairs");
        passedTests++;
        
        // Test a sample cost query with time conversion
        if (poiNodeIds.size() >= 2) {
            float sampleCostPx = costMatrix.GetCost(poiNodeIds[0], poiNodeIds[1]);
            double sampleDistM = PixelsToMeters(sampleCostPx, MAP_RESOLUTION);
            double sampleTimeS = MetersToSeconds(sampleDistM);
            
            std::cout << "  Sample: cost(" << poiNodeIds[0] << " -> " << poiNodeIds[1] << ") = "
                      << sampleCostPx << " px = " 
                      << std::fixed << std::setprecision(2) << sampleDistM << " m = "
                      << std::fixed << std::setprecision(2) << sampleTimeS << " s\n";
        }
    } else {
        PrintFail("No POI nodes found - cannot precompute cost matrix");
    }
    totalTests++;

    // =========================================================================
    // PHASE 3: Robot Agent Creation (with Battery System)
    // =========================================================================
    PrintHeader("PHASE 3: Robot Agent Creation (Battery: 300s full)");

    std::vector<RobotAgent> robots;
    
    int numRobots = 6;
    for (int i = 0; i < numRobots; ++i) {
        int chargingNode = chargingNodes.empty() ? 0 : chargingNodes[i % chargingNodes.size()];
        
        RobotAgent robot(
            i,                  // Robot ID
            1.0f,               // Battery capacity (normalized)
            chargingNode,       // Assigned charging station
            ROBOT_SPEED_MPS,    // Average speed (1.6 m/s)
            1                   // Capacity (1 packet at a time)
        );
        
        // Set initial position (at charging station)
        robot.SetCurrentNodeId(chargingNode);
        robot.SetBatteryLevel(1.0f);  // Full battery = 300 seconds
        
        robots.push_back(robot);
        
        std::cout << "  Created: " << robot << std::endl;
    }
    
    PrintPass("Created " + std::to_string(robots.size()) + " robot agents at charging stations");
    passedTests++;
    totalTests++;

    // =========================================================================
    // PHASE 4: Task Loading from API (POI IDs: P1, P2, etc.)
    // =========================================================================
    PrintHeader("PHASE 4: Task Loading from API (POI IDs)");

    std::vector<Task> tasks;
    
    const std::string taskFilePath = "../../api/set_of_tasks.json";
    PrintInfo("Loading tasks from: " + taskFilePath);
    PrintInfo("Using POI ID format: P1, P2, P3...");
    
    // Use the new POI-aware loader
    tasks = TaskLoader::LoadTasksWithPOI(taskFilePath, navMesh, poiRegistry);
    
    if (!tasks.empty()) {
        PrintPass("Loaded " + std::to_string(tasks.size()) + " tasks from API");
        passedTests++;
        
        // Print first 10 tasks for brevity
        std::cout << "\n  Loaded Tasks (first 10):\n";
        for (size_t i = 0; i < std::min(tasks.size(), size_t(10)); ++i) {
            std::cout << "    " << tasks[i] << std::endl;
        }
        if (tasks.size() > 10) {
            std::cout << "    ... and " << (tasks.size() - 10) << " more tasks\n";
        }
    } else {
        PrintFail("No tasks loaded from API file");
        return 1;
    }
    totalTests++;

    // =========================================================================
    // PHASE 5: VRP Solving (Hill Climbing)
    // =========================================================================
    PrintHeader("PHASE 5: VRP Solving (Hill Climbing)");

    if (!tasks.empty() && !robots.empty()) {
        // Precompute costs for task nodes
        std::vector<int> taskNodes;
        for (const auto& task : tasks) {
            taskNodes.push_back(task.sourceNode);
            taskNodes.push_back(task.destinationNode);
        }
        for (const auto& robot : robots) {
            taskNodes.push_back(robot.GetCurrentNodeId());
        }
        std::sort(taskNodes.begin(), taskNodes.end());
        taskNodes.erase(std::unique(taskNodes.begin(), taskNodes.end()), taskNodes.end());
        
        costMatrix.PrecomputeForNodes(taskNodes);
        
        // Create solver
        HillClimbing solver(100, 5, 42);
        
        // Solve VRP
        VRPResult result = solver.Solve(tasks, robots, costMatrix);
        
        auto computeEndTime = std::chrono::high_resolution_clock::now();
        computationTimeMs = std::chrono::duration<double, std::milli>(computeEndTime - computeStartTime).count();
        
        // Print result
        result.Print();
        
        if (result.isFeasible) {
            PrintPass("VRP solved successfully!");
            passedTests++;
            
            // Print itineraries
            std::cout << "\n  Robot Itineraries:\n";
            for (const auto& robot : robots) {
                const auto& itinerary = robot.GetItinerary();
                std::cout << "    Robot " << robot.GetRobotId() << ": [" << robot.GetCurrentNodeId();
                for (size_t i = 0; i < itinerary.size(); ++i) {
                    std::cout << " → " << itinerary[i];
                }
                std::cout << "] (" << itinerary.size() << " nodes)\n";
            }
            
            PrintPass("Itineraries assigned to all robots");
            passedTests++;
        } else {
            PrintFail("VRP solving failed - infeasible");
        }
    } else {
        PrintFail("Cannot solve VRP - no tasks or robots");
    }
    totalTests += 2;

    // =========================================================================
    // PHASE 6: Fleet Execution Simulation (with Battery)
    // =========================================================================
    PrintHeader("PHASE 6: Fleet Execution Simulation");
    
    SimulationResult simResult = SimulateFleetExecution(robots, costMatrix, MAP_RESOLUTION, poiRegistry);
    
    PrintPass("Fleet simulation completed");
    passedTests++;
    totalTests++;

    // =========================================================================
    // FINAL SUMMARY
    // =========================================================================
    PrintFinalSummary(simResult, computationTimeMs, passedTests, totalTests);

    return (passedTests == totalTests) ? 0 : 1;
}
