// layer1/src/NavMeshGenerator.cc

#include "NavMeshGenerator.hh"
#include "AbstractGrid.hh"
#include "Resolution.hh"
#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>

namespace Backend {
namespace Layer1 {

    // =========================================================================
    // CONFIGURATION
    // =========================================================================
    // The physical size of one graph node (robot footprint)
    // Robot is 60cm x 60cm = 0.6m x 0.6m
    // In DECIMETERS: 6 pixels wide, but we use 5x5 node grid for center-based logic
    constexpr float TARGET_NODE_SIZE_METERS = 0.5f;  // 5 decimeters = 50cm node spacing

    // =========================================================================
    // HELPER: Get step size in pixels based on resolution
    // =========================================================================
    static int GetStepSize(Backend::Common::Resolution res) {
        switch (res) {
            case Backend::Common::Resolution::DECIMETERS:
                // 1 pixel = 0.1 meter -> 5 pixels = 0.5m node (robot center)
                return 5;
            case Backend::Common::Resolution::CENTIMETERS:
                // 1 pixel = 0.01 meter -> 50 pixels = 0.5m node
                return 50;
            case Backend::Common::Resolution::MILLIMETERS:
                // 1 pixel = 0.001 meter -> 500 pixels = 0.5m node
                return 500;
            default:
                return 5; // Default to decimeters
        }
    }

    // =========================================================================
    // HELPER: Check if a tile is FULLY accessible (all pixels must be walkable)
    // AND if a robot can actually reach it (check clearance in all 4 directions)
    // =========================================================================
    static bool IsTileAccessible(const AbstractGrid& map, int tileX, int tileY, int stepSize) {
        auto dims = map.GetDimensions();
        int mapW = dims.first;
        int mapH = dims.second;
        
        // Check ALL pixels within the tile
        for (int dy = 0; dy < stepSize; ++dy) {
            for (int dx = 0; dx < stepSize; ++dx) {
                int px = tileX + dx;
                int py = tileY + dy;
                
                // Bounds check
                if (px >= mapW || py >= mapH) {
                    return false;
                }
                
                // If ANY pixel is an obstacle, tile is not accessible
                if (!map.IsAccessible({px, py})) {
                    return false;
                }
            }
        }
        
        // Additional check: Can a robot actually REACH this tile?
        // The robot needs at least ONE direction where it can approach with full clearance
        // Check if there's clearance to move INTO this tile from at least one cardinal direction
        
        bool canReachFromLeft = true;
        bool canReachFromRight = true;
        bool canReachFromTop = true;
        bool canReachFromBottom = true;
        
        // Check LEFT approach (robot moving from left needs corridor of stepSize width)
        if (tileX >= stepSize) {
            for (int dy = 0; dy < stepSize; ++dy) {
                for (int dx = -stepSize; dx < stepSize; ++dx) {
                    int px = tileX + dx;
                    int py = tileY + dy;
                    if (px < 0 || px >= mapW || py < 0 || py >= mapH || !map.IsAccessible({px, py})) {
                        canReachFromLeft = false;
                        break;
                    }
                }
                if (!canReachFromLeft) break;
            }
        } else {
            canReachFromLeft = false;
        }
        
        // Check RIGHT approach
        if (tileX + 2 * stepSize <= mapW) {
            for (int dy = 0; dy < stepSize; ++dy) {
                for (int dx = 0; dx < 2 * stepSize; ++dx) {
                    int px = tileX + dx;
                    int py = tileY + dy;
                    if (px >= mapW || py >= mapH || !map.IsAccessible({px, py})) {
                        canReachFromRight = false;
                        break;
                    }
                }
                if (!canReachFromRight) break;
            }
        } else {
            canReachFromRight = false;
        }
        
        // Check TOP approach
        if (tileY >= stepSize) {
            for (int dx = 0; dx < stepSize; ++dx) {
                for (int dy = -stepSize; dy < stepSize; ++dy) {
                    int px = tileX + dx;
                    int py = tileY + dy;
                    if (px >= mapW || py < 0 || py >= mapH || !map.IsAccessible({px, py})) {
                        canReachFromTop = false;
                        break;
                    }
                }
                if (!canReachFromTop) break;
            }
        } else {
            canReachFromTop = false;
        }
        
        // Check BOTTOM approach
        if (tileY + 2 * stepSize <= mapH) {
            for (int dx = 0; dx < stepSize; ++dx) {
                for (int dy = 0; dy < 2 * stepSize; ++dy) {
                    int px = tileX + dx;
                    int py = tileY + dy;
                    if (px >= mapW || py >= mapH || !map.IsAccessible({px, py})) {
                        canReachFromBottom = false;
                        break;
                    }
                }
                if (!canReachFromBottom) break;
            }
        } else {
            canReachFromBottom = false;
        }
        
        // Tile is only accessible if robot can reach it from at least ONE direction
        return canReachFromLeft || canReachFromRight || canReachFromTop || canReachFromBottom;
    }

    // =========================================================================
    // HELPER: Check if a robot can pass between two adjacent tiles
    // The ENTIRE robot body (stepSize x stepSize) must fit through the passage
    // =========================================================================
    static bool CanRobotPassBetween(const AbstractGrid& map, 
                                     int tile1X, int tile1Y, 
                                     int tile2X, int tile2Y, 
                                     int stepSize) {
        auto dims = map.GetDimensions();
        int mapW = dims.first;
        int mapH = dims.second;
        
        // Both tiles are already verified to be fully walkable
        // Now check if the robot can physically move between them
        
        // The robot occupies stepSize x stepSize during the entire movement
        // We need to verify ALL pixels the robot sweeps through
        
        int dx = tile2X - tile1X;  // +stepSize = moving right, -stepSize = moving left
        int dy = tile2Y - tile1Y;  // +stepSize = moving down, -stepSize = moving up
        
        if (dx != 0) {
            // Horizontal movement (left/right)
            // The robot sweeps a rectangle from tile1 to tile2
            int minX = std::min(tile1X, tile2X);
            int maxX = std::max(tile1X, tile2X) + stepSize;
            
            // Check the entire swept area (height = stepSize, width = 2*stepSize)
            for (int y = tile1Y; y < tile1Y + stepSize; ++y) {
                for (int x = minX; x < maxX; ++x) {
                    if (x < 0 || x >= mapW || y < 0 || y >= mapH) {
                        return false;
                    }
                    if (!map.IsAccessible({x, y})) {
                        return false;
                    }
                }
            }
        } else if (dy != 0) {
            // Vertical movement (up/down)
            // The robot sweeps a rectangle from tile1 to tile2
            int minY = std::min(tile1Y, tile2Y);
            int maxY = std::max(tile1Y, tile2Y) + stepSize;
            
            // Check the entire swept area (width = stepSize, height = 2*stepSize)
            for (int y = minY; y < maxY; ++y) {
                for (int x = tile1X; x < tile1X + stepSize; ++x) {
                    if (x < 0 || x >= mapW || y < 0 || y >= mapH) {
                        return false;
                    }
                    if (!map.IsAccessible({x, y})) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }

    // =========================================================================
    // MAIN COMPUTE FUNCTION - UNIFORM TILING
    // =========================================================================
    void NavMeshGenerator::ComputeRecast(const AbstractGrid& map, NavMesh& mesh) {
        auto dims = map.GetDimensions();
        int mapW = dims.first;
        int mapH = dims.second;
        
        Backend::Common::Resolution resolution = map.GetResolution();
        int stepSize = GetStepSize(resolution);

        std::cout << "[NavMeshGenerator] Starting Uniform Tiling..." << std::endl;
        std::cout << "[NavMeshGenerator] Map: " << mapW << "x" << mapH << " pixels" << std::endl;
        std::cout << "[NavMeshGenerator] Resolution: " 
                  << Backend::Common::GetResolutionName(resolution)
                  << " (Step Size: " << stepSize << " pixels = " << TARGET_NODE_SIZE_METERS << "m)" << std::endl;

        // Calculate expected grid dimensions in tiles
        int tilesX = mapW / stepSize;
        int tilesY = mapH / stepSize;
        std::cout << "[NavMeshGenerator] Expected Grid: " << tilesX << "x" << tilesY 
                  << " = " << (tilesX * tilesY) << " max tiles" << std::endl;

        // =====================================================================
        // PHASE 1: Create Nodes (Uniform Grid Sampling)
        // =====================================================================
        // Map from (tileGridX, tileGridY) -> nodeId for edge creation
        std::unordered_map<int, int> tileToNodeId;
        
        auto getTileKey = [tilesX](int gx, int gy) -> int {
            return gy * tilesX + gx;
        };

        int nodeIdCounter = 0;
        
        for (int tileGridY = 0; tileGridY < tilesY; ++tileGridY) {
            for (int tileGridX = 0; tileGridX < tilesX; ++tileGridX) {
                // Calculate pixel coordinates of tile's top-left
                int pixelX = tileGridX * stepSize;
                int pixelY = tileGridY * stepSize;
                
                // Check if tile is accessible
                if (IsTileAccessible(map, pixelX, pixelY, stepSize)) {
                    // Create node at tile center
                    int centerX = pixelX + stepSize / 2;
                    int centerY = pixelY + stepSize / 2;
                    
                    mesh.AddNode({centerX, centerY});
                    tileToNodeId[getTileKey(tileGridX, tileGridY)] = nodeIdCounter;
                    ++nodeIdCounter;
                }
            }
        }

        std::cout << "[NavMeshGenerator] Created " << nodeIdCounter << " nodes (tiles)" << std::endl;

        // =====================================================================
        // PHASE 2: Create Edges (4-Connected Neighbors with Robot Clearance Check)
        // =====================================================================
        int edgeCount = 0;
        
        // Direction offsets for 4-connectivity
        const int dx[] = {1, 0, -1, 0};  // Right, Down, Left, Up
        const int dy[] = {0, 1, 0, -1};
        
        for (int tileGridY = 0; tileGridY < tilesY; ++tileGridY) {
            for (int tileGridX = 0; tileGridX < tilesX; ++tileGridX) {
                int currentKey = getTileKey(tileGridX, tileGridY);
                
                // Skip if this tile has no node
                auto currentIt = tileToNodeId.find(currentKey);
                if (currentIt == tileToNodeId.end()) continue;
                
                int currentNodeId = currentIt->second;
                
                // Pixel coordinates of current tile
                int currentPixelX = tileGridX * stepSize;
                int currentPixelY = tileGridY * stepSize;
                
                // Check all 4 neighbors
                for (int d = 0; d < 4; ++d) {
                    int neighborGridX = tileGridX + dx[d];
                    int neighborGridY = tileGridY + dy[d];
                    
                    // Bounds check
                    if (neighborGridX < 0 || neighborGridX >= tilesX ||
                        neighborGridY < 0 || neighborGridY >= tilesY) {
                        continue;
                    }
                    
                    int neighborKey = getTileKey(neighborGridX, neighborGridY);
                    auto neighborIt = tileToNodeId.find(neighborKey);
                    
                    if (neighborIt != tileToNodeId.end()) {
                        int neighborNodeId = neighborIt->second;
                        
                        // Pixel coordinates of neighbor tile
                        int neighborPixelX = neighborGridX * stepSize;
                        int neighborPixelY = neighborGridY * stepSize;
                        
                        // Only add edge in one direction to avoid double-counting
                        if (currentNodeId < neighborNodeId) {
                            // CHECK: Can a robot actually pass between these tiles?
                            if (CanRobotPassBetween(map, currentPixelX, currentPixelY,
                                                    neighborPixelX, neighborPixelY, stepSize)) {
                                // Cost = Euclidean distance = stepSize for cardinal directions
                                float cost = static_cast<float>(stepSize);
                                
                                // Add bi-directional edges
                                mesh.AddEdge(currentNodeId, neighborNodeId, cost);
                                mesh.AddEdge(neighborNodeId, currentNodeId, cost);
                                ++edgeCount;
                            }
                        }
                    }
                }
            }
        }

        std::cout << "[NavMeshGenerator] Created " << edgeCount << " edges (bi-directional)" << std::endl;
        
        // =====================================================================
        // PHASE 3: Remove Orphan Nodes (nodes with no edges - unreachable)
        // =====================================================================
        int orphansRemoved = mesh.RemoveOrphanNodes();
        if (orphansRemoved > 0) {
            std::cout << "[NavMeshGenerator] Removed " << orphansRemoved << " unreachable nodes" << std::endl;
        }
        
        // Calculate physical dimensions for verification
        float physicalW = mapW * Backend::Common::GetConversionFactorToMeters(resolution);
        float physicalH = mapH * Backend::Common::GetConversionFactorToMeters(resolution);
        std::cout << "[NavMeshGenerator] Physical Map Size: " << physicalW << "m x " << physicalH << "m" << std::endl;
    }

} // namespace Layer1
} // namespace Backend