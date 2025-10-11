#include "include/Graph.hh"
#include <iostream>
#include <vector>
#include <string>

void processGraph(const std::string& inputFile, const std::string& outputFile, int graphNum) {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  Processing Graph " << graphNum << std::endl;
    std::cout << "========================================" << std::endl;
    
    Graph graph;
    
    if (graph.loadFromFile(inputFile)) {
        std::cout << "\n✓ Graph loaded successfully!" << std::endl;
        
        // Print summary
        std::cout << "\n--- Quick Summary ---" << std::endl;
        std::cout << "Total vertices: " << graph.getNumVertices() << std::endl;
        
        // Count node types
        int charging = 0, pickup = 0, dropoff = 0, waypoint = 0, afk = 0, forbidden = 0;
        for (int i = 0; i < 100; i++) {  // Check up to 100 nodes
            const Graph::Node* node = graph.getNode(i);
            if (node) {
                switch (node->type) {
                    case Graph::NodeType::Charging: charging++; break;
                    case Graph::NodeType::Pickup: pickup++; break;
                    case Graph::NodeType::Dropoff: dropoff++; break;
                    case Graph::NodeType::Waypoint: waypoint++; break;
                    case Graph::NodeType::AFK: afk++; break;
                    case Graph::NodeType::ForbiddenCorner: forbidden++; break;
                    default: break;
                }
            }
        }
        
        std::cout << "  Charging stations: " << charging << std::endl;
        std::cout << "  Pickup points: " << pickup << std::endl;
        std::cout << "  Dropoff points: " << dropoff << std::endl;
        std::cout << "  Waypoints: " << waypoint << std::endl;
        std::cout << "  AFK zones: " << afk << std::endl;
        std::cout << "  Forbidden corners: " << forbidden << std::endl;
        
        // Generate SVG
        std::cout << "\n--- Generating Visualization ---" << std::endl;
        if (graph.generateSVG(outputFile, 1200, 900)) {
            std::cout << "✓ Saved to: " << outputFile << std::endl;
        } else {
            std::cerr << "✗ Failed to generate visualization" << std::endl;
        }
        
    } else {
        std::cerr << "✗ Failed to load graph from " << inputFile << std::endl;
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   GRAPH VISUALIZATION GENERATOR       ║" << std::endl;
    std::cout << "║   Processing Multiple Graph Files     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    // Process all ten graphs
    std::vector<std::pair<std::string, std::string>> graphs = {
        {"tests/distributions/graph1.inp", "utils/output/graph1_visualization.svg"},
        {"tests/distributions/graph2.inp", "utils/output/graph2_visualization.svg"},
        {"tests/distributions/graph3.inp", "utils/output/graph3_visualization.svg"},
        {"tests/distributions/graph4.inp", "utils/output/graph4_visualization.svg"},
        {"tests/distributions/graph5.inp", "utils/output/graph5_visualization.svg"},
        {"tests/distributions/graph6.inp", "utils/output/graph6_visualization.svg"},
        {"tests/distributions/graph7.inp", "utils/output/graph7_visualization.svg"},
        {"tests/distributions/graph8.inp", "utils/output/graph8_visualization.svg"},
        {"tests/distributions/graph9.inp", "utils/output/graph9_visualization.svg"},
        {"tests/distributions/graph10.inp", "utils/output/graph10_visualization.svg"}
    };
    
    for (size_t i = 0; i < graphs.size(); i++) {
        processGraph(graphs[i].first, graphs[i].second, i + 1);
    }
    
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  ALL GRAPHS PROCESSED SUCCESSFULLY!   " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nOpen the following files in your browser:" << std::endl;
    std::cout << "  1. utils/output/graph1_visualization.svg" << std::endl;
    std::cout << "  2. utils/output/graph2_visualization.svg" << std::endl;
    std::cout << "  3. utils/output/graph3_visualization.svg" << std::endl;
    std::cout << "  4. utils/output/graph4_visualization.svg" << std::endl;
    std::cout << "  5. utils/output/graph5_visualization.svg" << std::endl;
    std::cout << "  6. utils/output/graph6_visualization.svg" << std::endl;
    std::cout << "  7. utils/output/graph7_visualization.svg" << std::endl;
    std::cout << "  8. utils/output/graph8_visualization.svg" << std::endl;
    std::cout << "  9. utils/output/graph9_visualization.svg" << std::endl;
    std::cout << " 10. utils/output/graph10_visualization.svg" << std::endl;
    std::cout << "\nOr open utils/viewer.html for all visualizations!" << std::endl;
    std::cout << "\n";
    
    return 0;
}