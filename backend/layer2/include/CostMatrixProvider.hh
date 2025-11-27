/**
 * @file CostMatrixProvider.hh
 * @brief Pre-computed cost matrix using A* on NavMesh
 * 
 * Provides O(1) cost lookups between any two POI nodes after
 * O(N² × (E + V log V)) offline precomputation.
 */

#ifndef LAYER2_COSTMATRIXPROVIDER_HH
#define LAYER2_COSTMATRIXPROVIDER_HH

#include "../../layer1/include/NavMesh.hh"
#include <unordered_map>
#include <vector>
#include <limits>
#include <utility>
#include <functional>
#include <cmath>

namespace Backend {
namespace Layer2 {

/**
 * @brief Hash function for pair<int,int> keys.
 */
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        // Combine the two integers into a single hash
        return std::hash<long long>()(
            (static_cast<long long>(p.first) << 32) | 
            static_cast<unsigned int>(p.second)
        );
    }
};

/**
 * @brief Pre-computes and caches travel costs between nodes.
 * 
 * This class runs A* algorithm on the NavMesh to compute the optimal
 * travel cost (distance or time) between all pairs of POI nodes.
 * 
 * Usage:
 * 1. Construct with NavMesh
 * 2. Call PrecomputeForNodes() with list of POI node IDs
 * 3. Query costs with GetCost()
 * 
 * The cost matrix is symmetric (undirected graph assumption).
 */
class CostMatrixProvider {
private:
    // Reference to the NavMesh for pathfinding
    const Backend::Layer1::NavMesh& navMesh_;
    
    // Cost matrix: (from, to) -> cost
    std::unordered_map<std::pair<int, int>, float, PairHash> costMatrix_;
    
    // Infinity constant for unreachable paths
    static constexpr float INFINITY_COST = std::numeric_limits<float>::max();

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct with NavMesh reference.
     * 
     * @param mesh The NavMesh to use for A* pathfinding
     */
    explicit CostMatrixProvider(const Backend::Layer1::NavMesh& mesh)
        : navMesh_(mesh) {}

    // =========================================================================
    // PRECOMPUTATION
    // =========================================================================
    
    /**
     * @brief Precompute costs between all pairs of specified nodes.
     * 
     * This runs A* for each source node to all destination nodes.
     * Time complexity: O(N² × (E + V log V)) where N = number of nodes.
     * 
     * For a typical warehouse with ~100 POIs and ~500 NavMesh nodes:
     * - 100² = 10,000 A* runs
     * - Each A* is O(500 + 500 log 500) ≈ O(5000)
     * - Total: ~50 million operations (fast on modern CPUs)
     * 
     * @param nodeIds Vector of node IDs (POIs) to precompute costs for
     * @return Number of pairs successfully computed
     */
    int PrecomputeForNodes(const std::vector<int>& nodeIds);
    
    /**
     * @brief Add costs from a single node to all POI nodes.
     * 
     * Used for ONLINE updates when a robot's current position
     * needs to be added to the cost matrix.
     * 
     * @param fromNodeId Source node (robot position)
     * @param toNodeIds Destination nodes (POIs)
     * @return Number of paths successfully computed
     */
    int AddRowForNode(int fromNodeId, const std::vector<int>& toNodeIds);

    // =========================================================================
    // QUERIES
    // =========================================================================
    
    /**
     * @brief Get the cost between two nodes.
     * 
     * @param fromNodeId Source node ID
     * @param toNodeId Destination node ID
     * @return Travel cost (distance), or INFINITY if unreachable
     */
    float GetCost(int fromNodeId, int toNodeId) const;
    
    /**
     * @brief Check if a path exists between two nodes.
     */
    bool HasPath(int fromNodeId, int toNodeId) const;
    
    /**
     * @brief Get the number of precomputed pairs.
     */
    size_t GetMatrixSize() const { return costMatrix_.size(); }
    
    /**
     * @brief Clear all cached costs.
     */
    void Clear() { costMatrix_.clear(); }

    // =========================================================================
    // A* ALGORITHM
    // =========================================================================
    
    /**
     * @brief Run A* from source to target on the NavMesh.
     * 
     * @param sourceId Source node ID
     * @param targetId Target node ID
     * @return Optimal path cost, or INFINITY if no path exists
     */
    float RunAStar(int sourceId, int targetId) const;
    
    /**
     * @brief Run Dijkstra from source to all reachable nodes.
     * 
     * More efficient than running A* N times when computing
     * costs from one node to many destinations.
     * 
     * @param sourceId Source node ID
     * @return Map of targetId -> cost for all reachable nodes
     */
    std::unordered_map<int, float> RunDijkstra(int sourceId) const;

    // =========================================================================
    // DEBUG
    // =========================================================================
    
    /**
     * @brief Print the cost matrix (for debugging).
     * 
     * @param nodeIds Nodes to include in the printout
     */
    void PrintMatrix(const std::vector<int>& nodeIds) const;
    
    /**
     * @brief Get infinity constant for external use.
     */
    static float GetInfinity() { return INFINITY_COST; }
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_COSTMATRIXPROVIDER_HH
