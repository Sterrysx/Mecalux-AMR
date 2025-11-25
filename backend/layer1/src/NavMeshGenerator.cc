// layer1/src/NavMeshGenerator.cc

#include "NavMeshGenerator.hh"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Backend {
namespace Layer1 {

    // INTERNAL HELPER STRUCT
    // Represents a walkable region we discovered
    struct RectangleRegion {
        int id;
        int x, y; // Top-left
        int w, h; // Dimensions
        
        Backend::Common::Coordinates GetCentroid() const {
            return { x + w / 2, y + h / 2 };
        }

        // Check if this rectangle shares a border with 'other'
        bool IsNeighbor(const RectangleRegion& other) const {
            // Check for overlap on X axis (Vertical Adjacency)
            bool xOverlap = (x < other.x + other.w) && (x + w > other.x);
            bool touchesVertical = (y + h == other.y) || (y == other.y + other.h);
            
            if (xOverlap && touchesVertical) return true;

            // Check for overlap on Y axis (Horizontal Adjacency)
            bool yOverlap = (y < other.y + other.h) && (y + h > other.y);
            bool touchesHorizontal = (x + w == other.x) || (x == other.x + other.w);

            if (yOverlap && touchesHorizontal) return true;

            return false;
        }
    };

    // =========================================================
    // MAIN COMPUTE FUNCTION
    // =========================================================
    void NavMeshGenerator::ComputeRecast(const StaticBitMap& map, NavMesh& mesh) {
        std::cout << "[NavMeshGenerator] Starting Rectangular Decomposition..." << std::endl;

        auto dims = map.GetDimensions();
        int mapW = dims.first;
        int mapH = dims.second;

        // 1. Create a working copy of the grid to mark "visited" areas
        // We use a flat vector<bool> where false = visited/obstacle, true = needs processing
        std::vector<bool> remainingSpace(mapW * mapH, false);

        // Initialize: Copy walkability from StaticBitMap
        for (int y = 0; y < mapH; ++y) {
            for (int x = 0; x < mapW; ++x) {
                if (map.IsAccessible({x, y})) {
                    remainingSpace[y * mapW + x] = true;
                }
            }
        }

        std::vector<RectangleRegion> regions;
        int regionIdCounter = 0;

        // 2. DECOMPOSITION LOOP
        // Greedily find the largest rectangles
        for (int y = 0; y < mapH; ++y) {
            for (int x = 0; x < mapW; ++x) {
                
                // If this cell is walkable and not yet part of a rectangle
                if (remainingSpace[y * mapW + x]) {
                    
                    // A. Find Max Width
                    int width = 0;
                    while (x + width < mapW && remainingSpace[y * mapW + (x + width)]) {
                        width++;
                    }

                    // B. Find Max Height for this Width
                    int height = 1;
                    bool canExpandDown = true;
                    while (y + height < mapH && canExpandDown) {
                        // Check the entire row below
                        for (int k = 0; k < width; ++k) {
                            if (!remainingSpace[(y + height) * mapW + (x + k)]) {
                                canExpandDown = false;
                                break;
                            }
                        }
                        if (canExpandDown) height++;
                    }

                    // C. Create the Region
                    RectangleRegion region;
                    region.id = regionIdCounter++;
                    region.x = x;
                    region.y = y;
                    region.w = width;
                    region.h = height;
                    regions.push_back(region);

                    // D. Mark these cells as visited (false) in remainingSpace
                    for (int dy = 0; dy < height; ++dy) {
                        for (int dx = 0; dx < width; ++dx) {
                            remainingSpace[(y + dy) * mapW + (x + dx)] = false;
                        }
                    }
                }
            }
        }

        std::cout << "[NavMeshGenerator] Decomposed map into " << regions.size() << " convex regions." << std::endl;

        // 3. GENERATE NODES
        // Convert our Regions into NavMesh Nodes
        for (const auto& reg : regions) {
            mesh.AddNode(reg.GetCentroid());
        }

        // 4. GENERATE EDGES (Connectivity)
        // Check adjacency between all regions (O(N^2))
        // Since N is usually small (< 500 for a warehouse), this is instant.
        int edgesCount = 0;
        for (size_t i = 0; i < regions.size(); ++i) {
            for (size_t j = i + 1; j < regions.size(); ++j) {
                
                if (regions[i].IsNeighbor(regions[j])) {
                    // Calculate Cost (Distance between centroids)
                    float cost = regions[i].GetCentroid().DistanceTo(regions[j].GetCentroid());

                    // Add Bi-Directional Edge
                    mesh.AddEdge(regions[i].id, regions[j].id, cost);
                    mesh.AddEdge(regions[j].id, regions[i].id, cost);
                    edgesCount++;
                }
            }
        }

        std::cout << "[NavMeshGenerator] Graph Built: " << edgesCount << " edges created." << std::endl;
    }

} // namespace Layer1
} // namespace Backend