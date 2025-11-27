/**
 * @file CostMatrixProvider.cc
 * @brief Implementation of A* and cost matrix computation
 */

#include "../include/CostMatrixProvider.hh"
#include <queue>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace Backend {
namespace Layer2 {

// =============================================================================
// PRECOMPUTATION
// =============================================================================

int CostMatrixProvider::PrecomputeForNodes(const std::vector<int>& nodeIds) {
    if (nodeIds.empty()) return 0;
    
    std::cout << "[CostMatrix] Precomputing costs for " << nodeIds.size() << " nodes..." << std::endl;
    
    int totalPairs = 0;
    
    // For each source node, run Dijkstra to get costs to all other nodes
    for (int sourceId : nodeIds) {
        auto distances = RunDijkstra(sourceId);
        
        // Store costs to all destination nodes
        for (int targetId : nodeIds) {
            if (sourceId == targetId) {
                // Same node = zero cost
                costMatrix_[{sourceId, targetId}] = 0.0f;
                totalPairs++;
            } else if (distances.count(targetId)) {
                costMatrix_[{sourceId, targetId}] = distances[targetId];
                totalPairs++;
            } else {
                // No path exists
                costMatrix_[{sourceId, targetId}] = INFINITY_COST;
            }
        }
    }
    
    std::cout << "[CostMatrix] Precomputed " << totalPairs << " node pairs" << std::endl;
    
    return totalPairs;
}

int CostMatrixProvider::AddRowForNode(int fromNodeId, const std::vector<int>& toNodeIds) {
    if (toNodeIds.empty()) return 0;
    
    // Run Dijkstra from this node
    auto distances = RunDijkstra(fromNodeId);
    
    int added = 0;
    for (int targetId : toNodeIds) {
        if (fromNodeId == targetId) {
            costMatrix_[{fromNodeId, targetId}] = 0.0f;
            added++;
        } else if (distances.count(targetId)) {
            costMatrix_[{fromNodeId, targetId}] = distances[targetId];
            added++;
        }
    }
    
    return added;
}

// =============================================================================
// QUERIES
// =============================================================================

float CostMatrixProvider::GetCost(int fromNodeId, int toNodeId) const {
    if (fromNodeId == toNodeId) return 0.0f;
    
    auto it = costMatrix_.find({fromNodeId, toNodeId});
    if (it != costMatrix_.end()) {
        return it->second;
    }
    
    // Not precomputed - run A* on demand
    return RunAStar(fromNodeId, toNodeId);
}

bool CostMatrixProvider::HasPath(int fromNodeId, int toNodeId) const {
    return GetCost(fromNodeId, toNodeId) < INFINITY_COST;
}

// =============================================================================
// A* ALGORITHM
// =============================================================================

float CostMatrixProvider::RunAStar(int sourceId, int targetId) const {
    if (sourceId == targetId) return 0.0f;
    
    const auto& allNodes = navMesh_.GetAllNodes();
    int numNodes = static_cast<int>(allNodes.size());
    
    if (sourceId < 0 || sourceId >= numNodes || 
        targetId < 0 || targetId >= numNodes) {
        return INFINITY_COST;
    }
    
    // Get target coordinates for heuristic
    const auto& targetCoords = allNodes[targetId].coords;
    
    // Priority queue: (f-score, nodeId)
    using PQEntry = std::pair<float, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> openSet;
    
    // g-score: cost from source to each node
    std::vector<float> gScore(numNodes, INFINITY_COST);
    gScore[sourceId] = 0.0f;
    
    // f-score = g + h (heuristic)
    auto heuristic = [&](int nodeId) -> float {
        const auto& coords = allNodes[nodeId].coords;
        return coords.DistanceTo(targetCoords);
    };
    
    openSet.push({heuristic(sourceId), sourceId});
    
    // Track visited nodes
    std::vector<bool> visited(numNodes, false);
    
    while (!openSet.empty()) {
        auto [fScore, current] = openSet.top();
        openSet.pop();
        
        if (current == targetId) {
            // Found the target
            return gScore[targetId];
        }
        
        if (visited[current]) continue;
        visited[current] = true;
        
        // Explore neighbors
        const auto& neighbors = navMesh_.GetNeighbors(current);
        for (const auto& edge : neighbors) {
            int neighbor = edge.targetNodeId;
            float tentativeG = gScore[current] + edge.cost;
            
            if (tentativeG < gScore[neighbor]) {
                gScore[neighbor] = tentativeG;
                float f = tentativeG + heuristic(neighbor);
                openSet.push({f, neighbor});
            }
        }
    }
    
    // No path found
    return INFINITY_COST;
}

std::unordered_map<int, float> CostMatrixProvider::RunDijkstra(int sourceId) const {
    std::unordered_map<int, float> distances;
    
    const auto& allNodes = navMesh_.GetAllNodes();
    int numNodes = static_cast<int>(allNodes.size());
    
    if (sourceId < 0 || sourceId >= numNodes) {
        return distances;
    }
    
    // Distance array
    std::vector<float> dist(numNodes, INFINITY_COST);
    dist[sourceId] = 0.0f;
    
    // Priority queue: (distance, nodeId)
    using PQEntry = std::pair<float, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;
    pq.push({0.0f, sourceId});
    
    // Visited tracking
    std::vector<bool> visited(numNodes, false);
    
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        // Store this distance
        distances[u] = d;
        
        // Explore neighbors
        const auto& neighbors = navMesh_.GetNeighbors(u);
        for (const auto& edge : neighbors) {
            int v = edge.targetNodeId;
            float newDist = d + edge.cost;
            
            if (newDist < dist[v]) {
                dist[v] = newDist;
                pq.push({newDist, v});
            }
        }
    }
    
    return distances;
}

// =============================================================================
// DEBUG
// =============================================================================

void CostMatrixProvider::PrintMatrix(const std::vector<int>& nodeIds) const {
    std::cout << "\n=== Cost Matrix ===\n";
    std::cout << std::setw(6) << " ";
    
    // Print header
    for (int to : nodeIds) {
        std::cout << std::setw(8) << to;
    }
    std::cout << "\n";
    
    // Print rows
    for (int from : nodeIds) {
        std::cout << std::setw(6) << from;
        for (int to : nodeIds) {
            float cost = GetCost(from, to);
            if (cost >= INFINITY_COST) {
                std::cout << std::setw(8) << "INF";
            } else {
                std::cout << std::setw(8) << std::fixed << std::setprecision(1) << cost;
            }
        }
        std::cout << "\n";
    }
    std::cout << "==================\n";
}

} // namespace Layer2
} // namespace Backend
