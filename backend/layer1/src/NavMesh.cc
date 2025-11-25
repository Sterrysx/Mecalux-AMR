#include "NavMesh.hh"
#include <cmath>
#include <limits>

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

} // namespace Layer1
} // namespace Backend