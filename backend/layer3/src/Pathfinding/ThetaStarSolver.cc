/**
 * @file ThetaStarSolver.cc
 * @brief Implementation of Theta* pathfinding algorithm
 */

#include "Pathfinding/ThetaStarSolver.hh"
#include <chrono>
#include <algorithm>
#include <iostream>

namespace Backend {
namespace Layer3 {
namespace Pathfinding {

// =============================================================================
// MAIN PATHFINDING
// =============================================================================

PathResult ThetaStarSolver::ComputePath(
    const Backend::Common::Coordinates& start,
    const Backend::Common::Coordinates& end,
    const Backend::Layer1::InflatedBitMap& safetyMap
) const {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PathResult result;
    result.success = false;
    result.pathLength = 0.0;
    result.nodesExpanded = 0;
    result.computeTimeMs = 0.0;
    
    // Get map dimensions
    auto [width, height] = safetyMap.GetDimensions();
    
    // Convert to grid coordinates
    int startX = SnapToGrid(start.x);
    int startY = SnapToGrid(start.y);
    int endX = SnapToGrid(end.x);
    int endY = SnapToGrid(end.y);
    
    // Clamp to map bounds
    startX = std::clamp(startX, 0, static_cast<int>(width - 1));
    startY = std::clamp(startY, 0, static_cast<int>(height - 1));
    endX = std::clamp(endX, 0, static_cast<int>(width - 1));
    endY = std::clamp(endY, 0, static_cast<int>(height - 1));
    
    // Check if start and end are accessible
    Backend::Common::Coordinates startCoord{startX, startY};
    Backend::Common::Coordinates endCoord{endX, endY};
    
    if (!safetyMap.IsAccessible(startCoord)) {
        std::cerr << "[ThetaStar] Start position is not accessible: " 
                  << start.x << ", " << start.y << "\n";
        result.computeTimeMs = 0.0;
        return result;
    }
    
    if (!safetyMap.IsAccessible(endCoord)) {
        std::cerr << "[ThetaStar] End position is not accessible: "
                  << end.x << ", " << end.y << "\n";
        result.computeTimeMs = 0.0;
        return result;
    }
    
    // Check for trivial case: direct line of sight
    if (HasLineOfSight(startX, startY, endX, endY, safetyMap)) {
        result.path.push_back(start);
        result.path.push_back(end);
        result.success = true;
        result.pathLength = Heuristic(startX, startY, endX, endY);
        result.nodesExpanded = 1;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.computeTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return result;
    }
    
    // Priority queue (min-heap by fCost)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    
    // Maps for tracking
    std::unordered_map<std::pair<int, int>, double, PairHash> gCosts;
    std::unordered_map<std::pair<int, int>, std::pair<int, int>, PairHash> parents;
    std::unordered_set<std::pair<int, int>, PairHash> closedSet;
    
    // Initialize start node
    Node startNode;
    startNode.x = startX;
    startNode.y = startY;
    startNode.gCost = 0.0;
    startNode.fCost = Heuristic(startX, startY, endX, endY);
    startNode.parentX = startX;
    startNode.parentY = startY;
    
    openSet.push(startNode);
    gCosts[{startX, startY}] = 0.0;
    parents[{startX, startY}] = {startX, startY};
    
    // A* main loop with Theta* modifications
    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();
        
        std::pair<int, int> currentKey = {current.x, current.y};
        
        // Skip if already processed
        if (closedSet.count(currentKey)) {
            continue;
        }
        
        closedSet.insert(currentKey);
        result.nodesExpanded++;
        
        // Goal check (with tolerance for grid snapping)
        if (std::abs(current.x - endX) <= GRID_STEP && 
            std::abs(current.y - endY) <= GRID_STEP) {
            // Reconstruct path
            result.path = ReconstructPath(current.x, current.y, parents, start, end);
            result.success = true;
            result.pathLength = gCosts[currentKey];
            
            // Simplify path for even smoother results
            result.path = SimplifyPath(result.path, safetyMap);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            result.computeTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            return result;
        }
        
        // Get parent of current
        auto parentIt = parents.find(currentKey);
        int parentX = (parentIt != parents.end()) ? parentIt->second.first : current.x;
        int parentY = (parentIt != parents.end()) ? parentIt->second.second : current.y;
        
        // Explore neighbors
        auto neighbors = GetNeighbors(current.x, current.y, safetyMap);
        
        for (const auto& [nx, ny] : neighbors) {
            std::pair<int, int> neighborKey = {nx, ny};
            
            if (closedSet.count(neighborKey)) {
                continue;
            }
            
            // Theta* key innovation: check line-of-sight to grandparent
            double tentativeG;
            int newParentX, newParentY;
            
            if (HasLineOfSight(parentX, parentY, nx, ny, safetyMap)) {
                // Path 2: Direct path from grandparent (Theta* optimization)
                tentativeG = gCosts[{parentX, parentY}] + Heuristic(parentX, parentY, nx, ny);
                newParentX = parentX;
                newParentY = parentY;
            } else {
                // Path 1: Standard A* (through current node)
                tentativeG = current.gCost + Heuristic(current.x, current.y, nx, ny);
                newParentX = current.x;
                newParentY = current.y;
            }
            
            // Check if this is a better path
            auto gIt = gCosts.find(neighborKey);
            if (gIt == gCosts.end() || tentativeG < gIt->second) {
                gCosts[neighborKey] = tentativeG;
                parents[neighborKey] = {newParentX, newParentY};
                
                Node neighborNode;
                neighborNode.x = nx;
                neighborNode.y = ny;
                neighborNode.gCost = tentativeG;
                neighborNode.fCost = tentativeG + Heuristic(nx, ny, endX, endY);
                neighborNode.parentX = newParentX;
                neighborNode.parentY = newParentY;
                
                openSet.push(neighborNode);
            }
        }
    }
    
    // No path found
    auto endTime = std::chrono::high_resolution_clock::now();
    result.computeTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cerr << "[ThetaStar] No path found from (" << startX << "," << startY 
              << ") to (" << endX << "," << endY << ")\n";
    
    return result;
}

// =============================================================================
// LINE OF SIGHT (Bresenham's Algorithm)
// =============================================================================

bool ThetaStarSolver::HasLineOfSight(
    int x1, int y1,
    int x2, int y2,
    const Backend::Layer1::InflatedBitMap& safetyMap
) const {
    // Bresenham's line algorithm
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (true) {
        // Check if current cell is accessible
        Backend::Common::Coordinates coord{x, y};
        if (!safetyMap.IsAccessible(coord)) {
            return false;
        }
        
        // Reached destination
        if (x == x2 && y == y2) {
            break;
        }
        
        int e2 = 2 * err;
        
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    
    return true;
}

// =============================================================================
// NEIGHBOR GENERATION
// =============================================================================

std::vector<std::pair<int, int>> ThetaStarSolver::GetNeighbors(
    int x, int y,
    const Backend::Layer1::InflatedBitMap& safetyMap
) const {
    std::vector<std::pair<int, int>> neighbors;
    neighbors.reserve(8);
    
    auto [width, height] = safetyMap.GetDimensions();
    
    // 8-connected neighbors
    static const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int i = 0; i < 8; ++i) {
        int nx = x + dx[i] * GRID_STEP;
        int ny = y + dy[i] * GRID_STEP;
        
        // Bounds check
        if (nx < 0 || nx >= static_cast<int>(width) ||
            ny < 0 || ny >= static_cast<int>(height)) {
            continue;
        }
        
        // Accessibility check
        Backend::Common::Coordinates coord{nx, ny};
        if (safetyMap.IsAccessible(coord)) {
            // For diagonal moves, also check the two adjacent cells
            if (dx[i] != 0 && dy[i] != 0) {
                // Diagonal move - check corner cutting
                Backend::Common::Coordinates adj1{x + dx[i] * GRID_STEP, y};
                Backend::Common::Coordinates adj2{x, y + dy[i] * GRID_STEP};
                if (safetyMap.IsAccessible(adj1) && safetyMap.IsAccessible(adj2)) {
                    neighbors.emplace_back(nx, ny);
                }
            } else {
                neighbors.emplace_back(nx, ny);
            }
        }
    }
    
    return neighbors;
}

// =============================================================================
// PATH RECONSTRUCTION
// =============================================================================

std::vector<Backend::Common::Coordinates> ThetaStarSolver::ReconstructPath(
    int endX, int endY,
    const std::unordered_map<std::pair<int, int>, std::pair<int, int>, PairHash>& parents,
    const Backend::Common::Coordinates& start,
    const Backend::Common::Coordinates& end
) const {
    std::vector<Backend::Common::Coordinates> path;
    
    int x = endX;
    int y = endY;
    
    // Trace back from end to start
    while (true) {
        Backend::Common::Coordinates coord{x, y};
        path.push_back(coord);
        
        auto it = parents.find({x, y});
        if (it == parents.end()) {
            break;
        }
        
        int px = it->second.first;
        int py = it->second.second;
        
        // Reached start
        if (px == x && py == y) {
            break;
        }
        
        x = px;
        y = py;
    }
    
    // Reverse to get start-to-end order
    std::reverse(path.begin(), path.end());
    
    // Replace first and last with exact coordinates
    if (!path.empty()) {
        path.front() = start;
        path.back() = end;
    }
    
    return path;
}

// =============================================================================
// PATH SIMPLIFICATION
// =============================================================================

std::vector<Backend::Common::Coordinates> ThetaStarSolver::SimplifyPath(
    const std::vector<Backend::Common::Coordinates>& path,
    const Backend::Layer1::InflatedBitMap& safetyMap
) const {
    if (path.size() <= 2) {
        return path;
    }
    
    std::vector<Backend::Common::Coordinates> simplified;
    simplified.push_back(path.front());
    
    size_t current = 0;
    
    while (current < path.size() - 1) {
        // Find the furthest point with line of sight
        size_t furthest = current + 1;
        
        for (size_t i = current + 2; i < path.size(); ++i) {
            if (HasLineOfSight(
                path[current].x,
                path[current].y,
                path[i].x,
                path[i].y,
                safetyMap
            )) {
                furthest = i;
            }
        }
        
        simplified.push_back(path[furthest]);
        current = furthest;
    }
    
    return simplified;
}

} // namespace Pathfinding
} // namespace Layer3
} // namespace Backend
