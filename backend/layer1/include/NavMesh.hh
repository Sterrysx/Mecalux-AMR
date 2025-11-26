#ifndef BACKEND_LAYER1_NAVMESH_HH
#define BACKEND_LAYER1_NAVMESH_HH

#include <vector>
#include <string>
#include "Coordinates.hh" // From common

// Since Node and Edge are simple structs defined in Common, we use them here.
// Ideally, create Node.hh and Edge.hh in Common, or define them here if specific to NavMesh.
// Based on your UML, they seem to be in Common/Shared Types.
// Assuming they are defined in a file like "GraphTypes.hh" or similar in common.
// For this implementation, I will define local structs if they aren't in common yet, 
// but based on your architecture, they should be imported.

namespace Backend {
namespace Common {
    // Re-declaring for context (remove if included via header)
    struct Node {
        Coordinates coords;
    };
    
    struct Edge {
        int targetNodeId;
        float cost;
    };
}

namespace Layer1 {

    class NavMesh {
    private:
        // The Nodes (Polygons/Centroids)
        std::vector<Backend::Common::Node> allNodes;

        // Adjacency List: Index = Source Node ID, Value = List of Edges
        std::vector<std::vector<Backend::Common::Edge>> adjacencyList;

    public:
        // Constructor
        NavMesh();

        // --- Graph Accessors ---
        const std::vector<Backend::Common::Node>& GetAllNodes() const;
        const std::vector<Backend::Common::Edge>& GetNeighbors(int nodeId) const;

        // --- Geometry Lookups ---
        
        // Converts a world coordinate (x,y) to the nearest NavMesh Node ID.
        // Returns -1 if not accessible or out of bounds.
        int GetNodeIdAt(Backend::Common::Coordinates coords) const;

        // --- Modifiers (Used by Generator) ---
        void AddNode(Backend::Common::Coordinates centroid);
        void AddEdge(int sourceId, int targetId, float cost);
        
        // Remove nodes that have no edges (unreachable)
        // Returns the number of nodes removed
        int RemoveOrphanNodes();

        // --- Export (For Layer 2 Integration / Visualization) ---
        // Exports graph topology to CSV for external analysis
        // Format: NodeID, CentroidX, CentroidY, Neighbors(ID:Cost|ID:Cost...)
        void ExportGraphToCSV(const std::string& filename) const;
    };

} // namespace Layer1
} // namespace Backend

#endif