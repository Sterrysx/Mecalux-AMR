#include <iostream>
#include "../01_layer_mapping/include/Graph.hh"
#include "include/Planifier.hh"
#include "include/Robot.hh"
using namespace std;

const int NUM_ROBOTS = 2;

int main() {
    // Create a simple test graph
    Graph G;
    
    // Add some test nodes
    G.addNode(0, Graph::NodeType::Charging, 0, 0);
    G.addNode(1, Graph::NodeType::Pickup, 10, 0);
    G.addNode(2, Graph::NodeType::Dropoff, 20, 0);
    G.addNode(3, Graph::NodeType::Waypoint, 30, 0);
    
    // Add some edges
    G.addEdge(0, 1);
    G.addEdge(1, 2);
    G.addEdge(2, 3);
    
    cout << "Graph created with " << G.getNumVertices() << " vertices" << endl;
    
    // Create planner
    Planifier P(G, NUM_ROBOTS);
    
    cout << "Planner created with " << P.getNumRobots() << " robots" << endl;
    
    // Run planning algorithm (interactive mode by default)
    P.plan();

    return 0;
}
