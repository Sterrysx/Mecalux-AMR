/**
 * =============================================================================
 * Layer 1 (Mapping) Integration Test Driver
 * =============================================================================
 * 
 * This test validates that the Layer 1 architecture holds together:
 * 1. Static Map Loading from file
 * 2. InflatedBitMap (Configuration Space) Generation
 * 3. NavMesh Graph Generation using InflatedBitMap
 * 4. POI Registry (Semantic Overlay) 
 * 5. Dynamic/Static isolation for Layer 2/3 separation
 * 
 * =============================================================================
 */

#include <iostream>
#include <cassert>
#include <string>

// Layer 1 includes
#include "StaticBitMap.hh"
#include "InflatedBitMap.hh"
#include "DynamicBitMap.hh"
#include "NavMesh.hh"
#include "NavMeshGenerator.hh"
#include "DynamicObstacleGenerator.hh"
#include "POIRegistry.hh"

// Common includes
#include "Coordinates.hh"
#include "Resolution.hh"

using namespace Backend::Layer1;
using namespace Backend::Common;

// =============================================================================
// ANSI Color Codes for test output
// =============================================================================
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

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

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================
int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     LAYER 1 (MAPPING) INTEGRATION TEST                        ║\n";
    std::cout << "║     Multi-Robot System - Backend Architecture Validation      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    int totalTests = 0;
    int passedTests = 0;

    // =========================================================================
    // TEST 1: Static Map Loading
    // =========================================================================
    PrintHeader("TEST 1: Static Map Loading");

    // Create StaticBitMap with DECIMETERS resolution
    // 300x200 pixels @ DECIMETERS = 30m x 20m physical warehouse
    // 1m node = 10 px step size -> 30x20 = 600 max tiles
    StaticBitMap staticMap(300, 200, Resolution::DECIMETERS);
    
    try {
        staticMap.LoadFromFile("assets/map_layout.txt");
        PrintPass("StaticBitMap loaded from file successfully");
        passedTests++;
    } catch (const std::exception& e) {
        PrintFail("Failed to load StaticBitMap: " + std::string(e.what()));
    }
    totalTests++;

    // Verify dimensions
    auto dims = staticMap.GetDimensions();
    std::cout << "  Map dimensions: " << dims.first << " x " << dims.second << std::endl;
    
    if (dims.first > 0 && dims.second > 0) {
        PrintPass("Map has valid dimensions");
        passedTests++;
    } else {
        PrintFail("Map has invalid dimensions");
    }
    totalTests++;

    // =========================================================================
    // TEST 2: InflatedBitMap (Configuration Space)
    // =========================================================================
    PrintHeader("TEST 2: InflatedBitMap (Configuration Space)");

    // Robot is 60cm wide, so radius = 30cm = 0.3m
    const float ROBOT_RADIUS_METERS = 0.3f;
    
    InflatedBitMap inflatedMap(staticMap, ROBOT_RADIUS_METERS);
    
    int origWalkable, inflatedWalkable, closedCells;
    inflatedMap.GetInflationStats(origWalkable, inflatedWalkable, closedCells);
    
    std::cout << "  Original walkable: " << origWalkable << std::endl;
    std::cout << "  After inflation: " << inflatedWalkable << std::endl;
    std::cout << "  Cells closed: " << closedCells << std::endl;
    std::cout << "  Inflation radius: " << inflatedMap.GetInflationRadiusPixels() << " pixels" << std::endl;

    if (closedCells > 0) {
        PrintPass("InflatedBitMap correctly closes cells near obstacles");
        passedTests++;
    } else {
        PrintFail("InflatedBitMap did not close any cells - check inflation logic");
    }
    totalTests++;

    // Export inflated map for visualization
    try {
        inflatedMap.ExportToFile("assets/inflated_map.txt");
        PrintPass("InflatedBitMap exported to assets/inflated_map.txt");
        passedTests++;
    } catch (...) {
        PrintFail("Failed to export InflatedBitMap");
    }
    totalTests++;

    // =========================================================================
    // TEST 3: NavMesh Graph Generation (using InflatedBitMap)
    // =========================================================================
    PrintHeader("TEST 3: NavMesh Graph Generation (Configuration Space)");

    NavMesh navMesh;
    NavMeshGenerator generator;
    
    // CRITICAL: Use InflatedBitMap instead of StaticBitMap for safe pathfinding
    generator.ComputeRecast(inflatedMap, navMesh);
    
    const auto& nodes = navMesh.GetAllNodes();
    std::cout << "  NavMesh nodes created: " << nodes.size() << std::endl;

    if (nodes.size() > 0) {
        PrintPass("NavMesh generated with " + std::to_string(nodes.size()) + " nodes (from inflated map)");
        passedTests++;
    } else {
        PrintFail("NavMesh is empty - no nodes generated!");
    }
    totalTests++;

    // Verify we can lookup a node
    if (nodes.size() > 0) {
        int testNodeId = navMesh.GetNodeIdAt(nodes[0].coords);
        if (testNodeId >= 0) {
            PrintPass("Node lookup by coordinate works");
            passedTests++;
        } else {
            PrintFail("Node lookup failed");
        }
        totalTests++;
    }

    // =========================================================================
    // TEST 3b: NavMesh Graph Export (For Layer 2 / Visualization)
    // =========================================================================
    PrintHeader("TEST 3b: NavMesh Graph Export");

    try {
        navMesh.ExportGraphToCSV("assets/graph_dump.csv");
        PrintPass("NavMesh exported to assets/graph_dump.csv");
        passedTests++;
    } catch (const std::exception& e) {
        PrintFail("Failed to export NavMesh: " + std::string(e.what()));
    }
    totalTests++;

    // =========================================================================
    // TEST 4: POI Registry (Semantic Overlay)
    // =========================================================================
    PrintHeader("TEST 4: POI Registry (Semantic Overlay)");

    POIRegistry poiRegistry;
    
    // Try to load from config file
    bool poiLoaded = poiRegistry.LoadFromJSON("assets/poi_config.json");
    
    if (poiLoaded) {
        PrintPass("POI configuration loaded from assets/poi_config.json");
        passedTests++;
    } else {
        std::cout << "  Note: POI config file not found, adding sample POIs manually\n";
        // Add sample POIs programmatically if file doesn't exist
        // CHARGING zones: C0, C1, ...
        poiRegistry.AddPOI("C0", POIType::CHARGING, {50, 20}, true);
        poiRegistry.AddPOI("C1", POIType::CHARGING, {55, 20}, true);
        // PICKUP zones: P0, P2, ... (robot collects packages)
        poiRegistry.AddPOI("P0", POIType::PICKUP, {10, 50}, true);
        // DROPOFF zones: P1, P3, ... (robot delivers packages)
        poiRegistry.AddPOI("P1", POIType::DROPOFF, {180, 50}, true);
        PrintPass("Sample POIs added manually");
        passedTests++;
    }
    totalTests++;

    // Validate POIs against the INFLATED MAP and map to NavMesh nodes
    // This ensures no POI is placed where the robot would clip a wall!
    int mappedCount = poiRegistry.ValidateAndMapToNavMesh(navMesh, inflatedMap);
    std::cout << "  POIs validated and mapped to NavMesh: " << mappedCount << std::endl;
    
    if (mappedCount > 0) {
        PrintPass("POIs validated against Configuration Space and mapped to NavMesh");
        passedTests++;
    } else {
        PrintFail("No POIs could be validated/mapped to NavMesh");
    }
    totalTests++;

    // Test type-based lookup
    auto chargingNodes = poiRegistry.GetNodesByType(POIType::CHARGING);
    auto pickupNodes = poiRegistry.GetNodesByType(POIType::PICKUP);
    auto dropoffNodes = poiRegistry.GetNodesByType(POIType::DROPOFF);
    
    std::cout << "  Charging nodes (Cx):  " << chargingNodes.size() << std::endl;
    std::cout << "  Pickup nodes (Px):    " << pickupNodes.size() << std::endl;
    std::cout << "  Dropoff nodes (Px):   " << dropoffNodes.size() << std::endl;

    if (chargingNodes.size() > 0 || pickupNodes.size() > 0 || dropoffNodes.size() > 0) {
        PrintPass("GetNodesByType() queries work correctly");
        passedTests++;
    } else {
        PrintFail("GetNodesByType() returned no results");
    }
    totalTests++;

    // Print POI summary
    poiRegistry.PrintSummary();

    // Export POI registry with node mappings
    poiRegistry.ExportToJSON("assets/poi_registry_export.json");

    // =========================================================================
    // TEST 5: Dynamic/Static Isolation (Critical for Layer 2/3 separation)
    // =========================================================================
    PrintHeader("TEST 5: Dynamic/Static Isolation");

    // Find a walkable coordinate for testing
    // Based on the map, (50, 50) should be in a walkable area
    Coordinates testCoord = {50, 50};
    
    // First, verify the test coordinate is walkable in the static map
    // If not, search for a walkable cell
    if (!staticMap.IsAccessible(testCoord)) {
        std::cout << "  Note: (50,50) is not walkable, searching for walkable cell...\n";
        bool found = false;
        for (int y = 3; y < dims.second - 3 && !found; ++y) {
            for (int x = 3; x < dims.first - 3 && !found; ++x) {
                if (staticMap.IsAccessible({x, y})) {
                    testCoord = {x, y};
                    found = true;
                }
            }
        }
        if (!found) {
            PrintFail("Could not find any walkable cell in the map!");
            return 1;
        }
    }
    
    std::cout << "  Test coordinate: (" << testCoord.x << ", " << testCoord.y << ")\n";

    // Verify it's walkable in static map
    bool staticAccessible = staticMap.IsAccessible(testCoord);
    std::cout << "  StaticBitMap.IsAccessible(" << testCoord.x << ", " << testCoord.y << "): " 
              << (staticAccessible ? "true" : "false") << std::endl;

    if (staticAccessible) {
        PrintPass("Test coordinate is accessible in StaticBitMap (Layer 2 view)");
        passedTests++;
    } else {
        PrintFail("Test coordinate should be accessible in StaticBitMap");
    }
    totalTests++;

    // Create DynamicBitMap from static
    DynamicBitMap dynamicMap(staticMap);

    // Verify it's initially also walkable in dynamic map
    bool dynamicAccessibleBefore = dynamicMap.IsAccessible(testCoord);
    std::cout << "  DynamicBitMap.IsAccessible (before obstacle): " 
              << (dynamicAccessibleBefore ? "true" : "false") << std::endl;

    if (dynamicAccessibleBefore) {
        PrintPass("DynamicBitMap initially matches StaticBitMap");
        passedTests++;
    } else {
        PrintFail("DynamicBitMap should initially match StaticBitMap");
    }
    totalTests++;

    // Create obstacle manager and spawn obstacle at test coordinate
    DynamicObstacleManager obstacleManager;
    obstacleManager.SpawnObstacleAt(testCoord, 1); // Size 1 obstacle

    std::cout << "  Spawned obstacle at (" << testCoord.x << ", " << testCoord.y << ")\n";

    // Update dynamic map with obstacles
    dynamicMap.Update(obstacleManager.GetActiveObstacles(), staticMap);

    // =======================================================================
    // CRITICAL ASSERTION: Verify Layer Isolation
    // =======================================================================
    bool staticAccessibleAfter = staticMap.IsAccessible(testCoord);
    bool dynamicAccessibleAfter = dynamicMap.IsAccessible(testCoord);

    std::cout << "\n" << COLOR_YELLOW << "  >>> CRITICAL ISOLATION TEST <<<" << COLOR_RESET << "\n";
    std::cout << "  StaticBitMap.IsAccessible (after obstacle spawn): " 
              << (staticAccessibleAfter ? "true" : "false") << std::endl;
    std::cout << "  DynamicBitMap.IsAccessible (after obstacle spawn): " 
              << (dynamicAccessibleAfter ? "true" : "false") << std::endl;

    // Static map MUST remain accessible (Layer 2 perspective - planning)
    if (staticAccessibleAfter) {
        PrintPass("StaticBitMap unchanged by dynamic obstacles (Layer 2 isolation OK)");
        passedTests++;
    } else {
        PrintFail("StaticBitMap was modified by dynamic obstacles - ISOLATION BROKEN!");
    }
    totalTests++;

    // Dynamic map MUST be blocked (Layer 3 perspective - real-time)
    if (!dynamicAccessibleAfter) {
        PrintPass("DynamicBitMap correctly blocked by obstacle (Layer 3 sees obstacles)");
        passedTests++;
    } else {
        PrintFail("DynamicBitMap not blocked - obstacle painting failed!");
    }
    totalTests++;

    // =======================================================================
    // FINAL ASSERTION: The layers are properly isolated
    // =======================================================================
    bool isolationValid = staticAccessibleAfter && !dynamicAccessibleAfter;
    
    std::cout << "\n";
    if (isolationValid) {
        std::cout << COLOR_GREEN 
                  << "  ✓ ASSERTION PASSED: Coordinate (" << testCoord.x << "," << testCoord.y 
                  << ") is Accessible in StaticBitMap but Blocked in DynamicBitMap"
                  << COLOR_RESET << "\n";
    } else {
        std::cout << COLOR_RED 
                  << "  ✗ ASSERTION FAILED: Layer isolation is broken!"
                  << COLOR_RESET << "\n";
    }

    // =========================================================================
    // TEST SUMMARY
    // =========================================================================
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      TEST SUMMARY                             ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    
    if (passedTests == totalTests) {
        std::cout << "║  " << COLOR_GREEN << "ALL TESTS PASSED: " << passedTests << "/" << totalTests 
                  << COLOR_RESET << "                                        ║\n";
        std::cout << "║  " << COLOR_GREEN << "Layer 1 Architecture: VALIDATED" 
                  << COLOR_RESET << "                              ║\n";
    } else {
        std::cout << "║  " << COLOR_RED << "TESTS PASSED: " << passedTests << "/" << totalTests 
                  << COLOR_RESET << "                                         ║\n";
        std::cout << "║  " << COLOR_RED << "Some tests failed - review output above" 
                  << COLOR_RESET << "                  ║\n";
    }
    
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    return (passedTests == totalTests) ? 0 : 1;
}
