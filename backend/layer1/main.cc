/**
 * =============================================================================
 * Layer 1 (Mapping) Integration Test Driver
 * =============================================================================
 * 
 * This test validates that the Layer 1 architecture holds together:
 * 1. Static Map Loading from file
 * 2. NavMesh Graph Generation
 * 3. Dynamic/Static isolation for Layer 2/3 separation
 * 
 * =============================================================================
 */

#include <iostream>
#include <cassert>
#include <string>

// Layer 1 includes
#include "StaticBitMap.hh"
#include "DynamicBitMap.hh"
#include "NavMesh.hh"
#include "NavMeshGenerator.hh"
#include "DynamicObstacleGenerator.hh"

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

    // Create StaticBitMap with placeholder dimensions (will be overwritten by file)
    StaticBitMap staticMap(300, 200, Resolution::CENTIMETERS);
    
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
    // TEST 2: NavMesh Graph Generation
    // =========================================================================
    PrintHeader("TEST 2: NavMesh Graph Generation");

    NavMesh navMesh;
    NavMeshGenerator generator;
    
    generator.ComputeRecast(staticMap, navMesh);
    
    const auto& nodes = navMesh.GetAllNodes();
    std::cout << "  NavMesh nodes created: " << nodes.size() << std::endl;

    if (nodes.size() > 0) {
        PrintPass("NavMesh generated with " + std::to_string(nodes.size()) + " nodes");
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
    // TEST 3: Dynamic/Static Isolation (Critical for Layer 2/3 separation)
    // =========================================================================
    PrintHeader("TEST 3: Dynamic/Static Isolation");

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
                  << COLOR_RESET << "                                   ║\n";
        std::cout << "║  " << COLOR_GREEN << "Layer 1 Architecture: VALIDATED" 
                  << COLOR_RESET << "                           ║\n";
    } else {
        std::cout << "║  " << COLOR_RED << "TESTS PASSED: " << passedTests << "/" << totalTests 
                  << COLOR_RESET << "                                         ║\n";
        std::cout << "║  " << COLOR_RED << "Some tests failed - review output above" 
                  << COLOR_RESET << "                  ║\n";
    }
    
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    return (passedTests == totalTests) ? 0 : 1;
}
