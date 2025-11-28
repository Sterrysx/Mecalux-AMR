/**
 * @file ThetaStarSolver.hh
 * @brief Theta* Pathfinding Algorithm for smooth any-angle paths
 * 
 * Theta* is an extension of A* that produces smooth paths by using
 * line-of-sight checks to skip unnecessary waypoints. This results
 * in paths that are more natural and shorter than grid-based A*.
 */

#ifndef LAYER3_PATHFINDING_THETASTARSOLVER_HH
#define LAYER3_PATHFINDING_THETASTARSOLVER_HH

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <functional>
#include <limits>

#include "Coordinates.hh"
#include "InflatedBitMap.hh"

namespace Backend {
namespace Layer3 {
namespace Pathfinding {

/**
 * @brief Result of a pathfinding request.
 */
struct PathResult {
    std::vector<Backend::Common::Coordinates> path;  ///< Waypoints from start to end
    bool success;                                     ///< Whether path was found
    double pathLength;                                ///< Total path length in pixels
    int nodesExpanded;                                ///< Number of nodes explored
    double computeTimeMs;                             ///< Computation time
};

/**
 * @brief Theta* pathfinding solver.
 * 
 * Theta* Algorithm:
 * 1. Like A*, maintain open and closed sets
 * 2. When updating a neighbor, check line-of-sight (LOS) to grandparent
 * 3. If LOS exists: set grandparent as parent (skipping intermediate node)
 * 4. If no LOS: set current node as parent (like regular A*)
 * 
 * This produces smooth "any-angle" paths instead of jagged grid paths.
 * 
 * Uses the InflatedBitMap from Layer 1 for safety checks (robot clearance).
 */
class ThetaStarSolver {
private:
    // Grid step size for node generation
    static constexpr int GRID_STEP = 5;  // 5 pixels = 0.5m at DECIMETERS resolution
    
    /**
     * @brief Internal node for pathfinding.
     */
    struct Node {
        int x, y;              ///< Grid coordinates
        double gCost;          ///< Cost from start
        double fCost;          ///< gCost + heuristic
        int parentX, parentY;  ///< Parent node coordinates
        
        bool operator>(const Node& other) const {
            return fCost > other.fCost;
        }
    };
    
    /**
     * @brief Hash function for (x, y) pairs.
     */
    struct PairHash {
        size_t operator()(const std::pair<int, int>& p) const {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
        }
    };

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Default constructor.
     */
    ThetaStarSolver() = default;

    // =========================================================================
    // MAIN INTERFACE
    // =========================================================================
    
    /**
     * @brief Compute a smooth path from start to end.
     * 
     * @param start Starting position (pixel coordinates)
     * @param end Goal position (pixel coordinates)
     * @param safetyMap Inflated bitmap for collision checking
     * @return PathResult containing the path and metadata
     */
    PathResult ComputePath(
        const Backend::Common::Coordinates& start,
        const Backend::Common::Coordinates& end,
        const Backend::Layer1::InflatedBitMap& safetyMap
    ) const;

    // =========================================================================
    // LINE OF SIGHT
    // =========================================================================
    
    /**
     * @brief Check if there is line-of-sight between two points.
     * 
     * Uses Bresenham's line algorithm to check all cells between
     * the two points against the safety map.
     * 
     * @param x1, y1 First point (pixels)
     * @param x2, y2 Second point (pixels)
     * @param safetyMap Map to check for obstacles
     * @return true if path is clear
     */
    bool HasLineOfSight(
        int x1, int y1,
        int x2, int y2,
        const Backend::Layer1::InflatedBitMap& safetyMap
    ) const;

private:
    // =========================================================================
    // INTERNAL HELPERS
    // =========================================================================
    
    /**
     * @brief Euclidean heuristic.
     */
    double Heuristic(int x1, int y1, int x2, int y2) const {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief Get 8-connected neighbors.
     */
    std::vector<std::pair<int, int>> GetNeighbors(
        int x, int y,
        const Backend::Layer1::InflatedBitMap& safetyMap
    ) const;
    
    /**
     * @brief Reconstruct path from parent map.
     */
    std::vector<Backend::Common::Coordinates> ReconstructPath(
        int endX, int endY,
        const std::unordered_map<std::pair<int, int>, std::pair<int, int>, PairHash>& parents,
        const Backend::Common::Coordinates& start,
        const Backend::Common::Coordinates& end
    ) const;
    
    /**
     * @brief Snap coordinate to grid.
     */
    int SnapToGrid(int coord) const {
        return (coord / GRID_STEP) * GRID_STEP;
    }
    
    /**
     * @brief Post-process path to remove redundant waypoints.
     */
    std::vector<Backend::Common::Coordinates> SimplifyPath(
        const std::vector<Backend::Common::Coordinates>& path,
        const Backend::Layer1::InflatedBitMap& safetyMap
    ) const;
};

} // namespace Pathfinding
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_PATHFINDING_THETASTARSOLVER_HH
