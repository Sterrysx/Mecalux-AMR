#include "include/Graph.hh"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
using namespace std;

void processGraph(const string graphFile, const string& outputFile, int graphNum) {
    cout << "\n";
    cout << "========================================" << endl;
    cout << "  Processing Graph " << graphNum << endl;
    cout << "========================================" << endl;
    
    Graph graph;
    ifstream inputFile(graphFile);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file " << graphFile << endl;
        return ;
    }
    
    // Create graph using efficient loadFromStream method

    graph.loadFromStream(inputFile);
    
    inputFile.close();
    cout << "\n✓ Graph loaded successfully!" << endl;
    
    // Print summary
    cout << "\n--- Quick Summary ---" << endl;
    cout << "Total vertices: " << graph.getNumVertices() << endl;
    
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
    
    cout << "  Charging stations: " << charging << endl;
    cout << "  Pickup points: " << pickup << endl;
    cout << "  Dropoff points: " << dropoff << endl;
    cout << "  Waypoints: " << waypoint << endl;
    cout << "  AFK zones: " << afk << endl;
    cout << "  Forbidden corners: " << forbidden << endl;
    
    // Generate SVG
    cout << "\n--- Generating Visualization ---" << endl;
    if (graph.generateSVG(outputFile, 1200, 900)) {
        cout << "✓ Saved to: " << outputFile << endl;
    } else {
        cerr << "✗ Failed to generate visualization" << endl;
    }
        
}

int main() {
    cout << "\n";
    cout << "╔════════════════════════════════════════╗" << endl;
    cout << "║   GRAPH VISUALIZATION GENERATOR       ║" << endl;
    cout << "║   Processing Multiple Graph Files     ║" << endl;
    cout << "╚════════════════════════════════════════╝" << endl;
    
    // Process all ten graphs
    vector<pair<string, string>> graphs = {
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
    
    cout << "\n";
    cout << "========================================" << endl;
    cout << "  ALL GRAPHS PROCESSED SUCCESSFULLY!   " << endl;
    cout << "========================================" << endl;
    cout << "\nOpen the following files in your browser:" << endl;
    cout << "  1. utils/output/graph1_visualization.svg" << endl;
    cout << "  2. utils/output/graph2_visualization.svg" << endl;
    cout << "  3. utils/output/graph3_visualization.svg" << endl;
    cout << "  4. utils/output/graph4_visualization.svg" << endl;
    cout << "  5. utils/output/graph5_visualization.svg" << endl;
    cout << "  6. utils/output/graph6_visualization.svg" << endl;
    cout << "  7. utils/output/graph7_visualization.svg" << endl;
    cout << "  8. utils/output/graph8_visualization.svg" << endl;
    cout << "  9. utils/output/graph9_visualization.svg" << endl;
    cout << " 10. utils/output/graph10_visualization.svg" << endl;
    cout << "\nOr open utils/viewer.html for all visualizations!" << endl;
    cout << "\n";
    
    return 0;
}