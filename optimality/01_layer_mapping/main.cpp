#include "include/Graph.hh"
#include <iostream>

int main() {
    std::cout << "=== Graph Loading Demo ===" << std::endl;
    
    // Create a graph instance
    Graph graph;
    
    // Load the graph from file
    std::string filename = "include/graph1.inp";
    
    if (graph.loadFromFile(filename)) {
        std::cout << "\nGraph loaded successfully!" << std::endl;
        
        // Print the complete graph
        graph.printGraph();
        
        // Demonstrate some queries
        std::cout << "\n=== GRAPH QUERIES ===" << std::endl;
        
        // Check specific nodes
        const Graph::Node* node14 = graph.getNode(14);
        if (node14) {
            std::cout << "Node 14 is at coordinates (" 
                      << node14->coordinates.first << ", " 
                      << node14->coordinates.second << ")" << std::endl;
        }
        
        // Show connections from node 14 (seems to be a central hub)
        const auto& edges14 = graph.getEdges(14);
        std::cout << "Node 14 has " << edges14.size() << " connections:" << std::endl;
        for (const auto& edge : edges14) {
            std::cout << "  -> Node " << edge.nodeDestinationId 
                      << " (distance: " << edge.distance << ")" << std::endl;
        }
        
        // Check charging stations (type C)
        std::cout << "\nCharging stations found:" << std::endl;
        for (int i = 0; i < graph.getNumVertices(); i++) {
            const Graph::Node* node = graph.getNode(i);
            if (node && node->type == Graph::NodeType::Charging) {
                std::cout << "  Node " << node->nodeId << " at (" 
                          << node->coordinates.first << ", " 
                          << node->coordinates.second << ")" << std::endl;
            }
        }
        
        // Generate SVG visualization
        std::cout << "\n=== GENERATING VISUALIZATION ===" << std::endl;
        if (graph.generateSVG("graph_visualization.svg", 1200, 900)) {
            std::cout << "Open 'graph_visualization.svg' in your browser to view the graph!" << std::endl;
        } else {
            std::cerr << "Failed to generate SVG visualization" << std::endl;
        }
        
    } else {
        std::cerr << "Failed to load graph from " << filename << std::endl;
        return 1;
    }
    
    return 0;
}