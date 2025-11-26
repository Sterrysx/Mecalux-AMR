#include "NavMesh.hh"
#include <cmath>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Backend {
namespace Layer1 {

    NavMesh::NavMesh() {}

    const std::vector<Backend::Common::Node>& NavMesh::GetAllNodes() const {
        return allNodes;
    }

    const std::vector<Backend::Common::Edge>& NavMesh::GetNeighbors(int nodeId) const {
        if (nodeId < 0 || nodeId >= (int)adjacencyList.size()) {
            static const std::vector<Backend::Common::Edge> empty;
            return empty;
        }
        return adjacencyList[nodeId];
    }

    int NavMesh::GetNodeIdAt(Backend::Common::Coordinates coords) const {
        // Naive implementation: Linear Search for nearest centroid.
        // OPTIMIZATION: In production, use a Spatial Hash, QuadTree, or 2D Grid Lookup.
        
        int bestNode = -1;
        float minDist = std::numeric_limits<float>::max();

        for (int i = 0; i < (int)allNodes.size(); ++i) {
            float dist = coords.DistanceTo(allNodes[i].coords);
            
            // Simple threshold: If we are close enough to a node center
            // (You might want strict polygon containment checks here later)
            if (dist < minDist) {
                minDist = dist;
                bestNode = i;
            }
        }
        
        // Threshold check (e.g., must be within 5 units of a node)
        // if (minDist > 5.0f) return -1; 

        return bestNode;
    }

    void NavMesh::AddNode(Backend::Common::Coordinates centroid) {
        Backend::Common::Node n;
        n.coords = centroid;
        allNodes.push_back(n);
        
        // Resize adjacency list to match node count
        adjacencyList.resize(allNodes.size());
    }

    void NavMesh::AddEdge(int sourceId, int targetId, float cost) {
        if (sourceId >= 0 && sourceId < (int)adjacencyList.size()) {
            Backend::Common::Edge e;
            e.targetNodeId = targetId;
            e.cost = cost;
            adjacencyList[sourceId].push_back(e);
        }
    }

    int NavMesh::RemoveOrphanNodes() {
        if (allNodes.empty()) return 0;
        
        // Use BFS to find all nodes reachable from node 0 (the main connected component)
        std::vector<bool> reachable(allNodes.size(), false);
        std::vector<int> queue;
        
        // Start BFS from node 0
        queue.push_back(0);
        reachable[0] = true;
        
        while (!queue.empty()) {
            int current = queue.back();
            queue.pop_back();
            
            // Visit all neighbors
            if (current < (int)adjacencyList.size()) {
                for (const auto& edge : adjacencyList[current]) {
                    if (edge.targetNodeId >= 0 && 
                        edge.targetNodeId < (int)reachable.size() &&
                        !reachable[edge.targetNodeId]) {
                        reachable[edge.targetNodeId] = true;
                        queue.push_back(edge.targetNodeId);
                    }
                }
            }
        }
        
        // Count unreachable nodes and build mapping from old ID to new ID
        std::vector<int> oldToNew(allNodes.size(), -1);
        int newId = 0;
        int orphanCount = 0;
        
        for (size_t i = 0; i < reachable.size(); ++i) {
            if (reachable[i]) {
                oldToNew[i] = newId++;
            } else {
                orphanCount++;
            }
        }
        
        if (orphanCount == 0) {
            return 0;  // No orphans to remove
        }
        
        // Build new node list (only reachable nodes)
        std::vector<Backend::Common::Node> newNodes;
        newNodes.reserve(newId);
        for (size_t i = 0; i < allNodes.size(); ++i) {
            if (oldToNew[i] != -1) {
                newNodes.push_back(allNodes[i]);
            }
        }
        
        // Build new adjacency list with remapped IDs
        std::vector<std::vector<Backend::Common::Edge>> newAdjList(newId);
        for (size_t i = 0; i < adjacencyList.size(); ++i) {
            if (oldToNew[i] != -1) {
                for (const auto& edge : adjacencyList[i]) {
                    if (oldToNew[edge.targetNodeId] != -1) {
                        Backend::Common::Edge newEdge;
                        newEdge.targetNodeId = oldToNew[edge.targetNodeId];
                        newEdge.cost = edge.cost;
                        newAdjList[oldToNew[i]].push_back(newEdge);
                    }
                }
            }
        }
        
        // Replace with cleaned data
        allNodes = std::move(newNodes);
        adjacencyList = std::move(newAdjList);
        
        return orphanCount;
    }

    void NavMesh::ExportGraphToCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[NavMesh] ERROR: Failed to open file for export: " << filename << std::endl;
            return;
        }

        // Write header comment
        file << "# NavMesh Graph Export\n";
        file << "# Format: NodeID, CentroidX, CentroidY, Neighbors(ID:Cost|ID:Cost...)\n";

        // Export each node with its neighbors
        for (size_t nodeId = 0; nodeId < allNodes.size(); ++nodeId) {
            const auto& node = allNodes[nodeId];
            
            // Write NodeID, CentroidX, CentroidY
            file << nodeId << ", " << node.coords.x << ", " << node.coords.y << ", ";
            
            // Write neighbors
            if (nodeId < adjacencyList.size()) {
                const auto& neighbors = adjacencyList[nodeId];
                for (size_t i = 0; i < neighbors.size(); ++i) {
                    file << neighbors[i].targetNodeId << ":" << neighbors[i].cost;
                    if (i < neighbors.size() - 1) {
                        file << "|";
                    }
                }
            }
            
            file << "\n";
        }

        file.close();
        std::cout << "[NavMesh] Exported " << allNodes.size() << " nodes to: " << filename << std::endl;
    }

} // namespace Layer1
} // namespace Backend