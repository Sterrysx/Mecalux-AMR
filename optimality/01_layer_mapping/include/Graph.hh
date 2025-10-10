#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <string>

//Where node is 
class Graph {

public:
    //NODE TYPE ENUM
    enum class NodeType {
        Waypoint = 0,
        Charging,
        AFK,
        Pickup,
        Dropoff,
        ObstacleCorner,
        ForbiddenCorner
    };

    //NODE STRUCT
    struct Node {
        int nodeId { -1 };
        NodeType type { NodeType::Waypoint };
        std::pair<double, double> coordinates { 0.0, 0.0 };
        Node() = default;
        Node(int id, NodeType t, double x, double y) : nodeId(id), type(t), coordinates{x,y} {}
    };

    //EDGE STRUCT
    struct Edge {
        double speed { 1.6 };     // edge weight as requested
        double distance { 0.0 };
        int nodeDestinationId { -1 };
        Edge() = default;
        Edge(double s, double d, int destId) : speed(s), distance(d), nodeDestinationId(destId) {}
    };

    //Constructor and Destructor
    Graph();
    ~Graph();

    //Helper Functions
    int addNode(int nodeId, NodeType type, double x, double y);
    bool addEdge(int fromNodeId, int toNodeId, double distance = 0.0, double speed = 1.6);
    const Node* getNode(int nodeId) const;
    const std::vector<Edge>& getEdges(int nodeId) const;
    int getNumVertices() const;
    void printGraph() const;
    Graph read_graph();
    
    // File I/O methods
    void loadFromStream(std::istream& in);
    bool loadFromFile(const std::string& filename);
    NodeType charToNodeType(char c) const;
    
    // Visualization methods
    bool generateSVG(const std::string& filename, int width = 800, int height = 600) const;

private:
    //Private Members
    int numVertices;
    int nextNodeId;

    //Graph Definition
    std::unordered_map<int, Node> nodes;  // Store nodes by ID
    std::vector<std::vector<Edge>> adjacencyList;  // Adjacency list representation
};

#endif // GRAPH_H