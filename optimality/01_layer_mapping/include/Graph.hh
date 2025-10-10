#ifndef GRAPH_H
#define GRAPH_H

using namespace std;
#include <bits/stdc++.h>

//Where node is 
class Graph {

public:
    //Constructor and Destructor
    Graph();
    ~Graph();

    //Helper Functions


private:
    //Private Members
    int numVertices;
    int nextNodeId;


    //NODE
    enum class NodeType {
        Waypoint = 0,
        Charging,
        AFK,
        Pickup,
        Dropoff,
        ObstacleCorner,
        ForbiddenCorner
    };

    struct Node {
        int nodeId { -1 };
        NodeType type { NodeType::Waypoint };
        pair<double, double> coordinates { {0.0, 0.0} };
        Node() = default;
        Node(int id, NodeType t, double x, double y) : nodeId(id), type(t), coordinates{x,y} {};
    };
    //NODE

    //EDGE
    struct Edge {
        double speed { 1.6 };     // edge weight as requested
        double distance { 0.0 };
        int nodeDestinationId { -1 };
        Edge() = default;
        Edge(double s, double d, int destId) : speed(s), distance(d), nodeDestinationId(destId) {};
    };
    //EDGE

    //Graph Definition
    priority_queue<int, set<Edge>, greater<int>> Graph;
};

#endif // GRAPH_H