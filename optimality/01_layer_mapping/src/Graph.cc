#include "../include/Graph.hh"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <set>
using namespace std;


// Constructor
Graph::Graph() : numVertices(0), nextNodeId(0) {
    // Initialize empty graph
    cout << "Graph constructor called" << endl;
}

// Destructor
Graph::~Graph() {
    cout << "Graph destructor called" << endl;
}


// Load graph from input stream (optimized - no temporary objects)
void Graph::loadFromStream(istream& in) {
    int num_vertices, num_edges;
    pair<int, int> mapSize;
    in >> num_vertices >> num_edges >> mapSize.first >> mapSize.second;

    // Read nodes
    int nodeId;
    pair<double, double> coords;
    string type;
    for (int i = 0; i < num_vertices; i++) {
        in >> nodeId >> coords.first >> coords.second >> type;
        addNode(nodeId, charToNodeType(type[0]), coords.first, coords.second);
    }

    // Read edges
    int fromNode, toNode;
    for (int i = 0; i < num_edges; i++) {
        in >> fromNode >> toNode;
        addEdge(fromNode, toNode);
        addEdge(toNode, fromNode);
    }
}


void Graph::read_graph() {
    int num_vertices, num_edges;
    pair<int, int> mapSize;
    cin >> num_vertices >> num_edges >> mapSize.first >> mapSize.second;

    // Read nodes
    int nodeId;
    pair<double, double> coords;
    string type;
    for (int i = 0; i < num_vertices; i++) {
        cin >> nodeId >> coords.first >> coords.second >> type;
        addNode(nodeId, charToNodeType(type[0]), coords.first, coords.second);
    }

    // Read edges
    int fromNode, toNode;
    for (int i = 0; i < num_edges; i++) {
        cin >> fromNode >> toNode;
        addEdge(fromNode, toNode);
        addEdge(toNode, fromNode);
    }


}


// Convert character to NodeType
Graph::NodeType Graph::charToNodeType(char c) const {
    switch (c) {
        case 'C': return NodeType::Charging;
        case 'A': return NodeType::AFK;
        case 'P': return NodeType::Pickup;
        case 'D': return NodeType::Dropoff;
        case 'W': return NodeType::Waypoint;
        case 'F': return NodeType::ForbiddenCorner;
        case 'O': return NodeType::ObstacleCorner;
        default: return NodeType::Waypoint;
    }
}

// Add a node with specific ID
int Graph::addNode(int nodeId, NodeType type, double x, double y) {
    Node newNode(nodeId, type, x, y);
    nodes[nodeId] = newNode;
    
    // Initialize empty set of edges for this node if it doesn't exist
    if (adjacencyList.find(nodeId) == adjacencyList.end()) {
        adjacencyList[nodeId] = set<Edge>();
    }
    
    numVertices++;
    if (nodeId >= nextNodeId) {
        nextNodeId = nodeId + 1;
    }
    
    return nodeId;
}

// Add an edge between two nodes
bool Graph::addEdge(int fromNodeId, int toNodeId, double distance, double speed) {
    // Check if both nodes exist
    if (nodes.find(fromNodeId) == nodes.end() || nodes.find(toNodeId) == nodes.end()) {
        return false;
    }
    
    // Calculate distance if not provided
    if (distance == 0.0) {
        const Node* fromNode = getNode(fromNodeId);
        const Node* toNode = getNode(toNodeId);
        if (fromNode && toNode) {
            double dx = fromNode->coordinates.first - toNode->coordinates.first;
            double dy = fromNode->coordinates.second - toNode->coordinates.second;
            distance = sqrt(dx * dx + dy * dy);
        }
    }
    
    // Add edges to both nodes (undirected graph)
    Edge newEdge1(speed, distance, toNodeId);
    Edge newEdge2(speed, distance, fromNodeId);
    
    adjacencyList[fromNodeId].insert(newEdge1);
    adjacencyList[toNodeId].insert(newEdge2);
    
    return true;
}

// Get node by ID
const Graph::Node* Graph::getNode(int nodeId) const {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Get all edges from a node
const vector<Graph::Edge>& Graph::getEdges(int nodeId) const {
    static vector<Edge> result;
    
    auto it = adjacencyList.find(nodeId);
    if (it != adjacencyList.end()) {
        result.clear();
        result.insert(result.end(), it->second.begin(), it->second.end());

    }
    return result;
}

// Get number of vertices
int Graph::getNumVertices() const {
    return numVertices;
}

// Load graph from file

// Generate SVG visualization of the graph
bool Graph::generateSVG(const string& filename, int width, int height) const {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create SVG file " << filename << endl;
        return false;
    }
    
    // Find min/max coordinates for scaling
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool first = true;
    
    for (const auto& [nodeId, node] : nodes) {
        if (first) {
            minX = maxX = node.coordinates.first;
            minY = maxY = node.coordinates.second;
            first = false;
        } else {
            minX = min(minX, node.coordinates.first);
            maxX = max(maxX, node.coordinates.first);
            minY = min(minY, node.coordinates.second);
            maxY = max(maxY, node.coordinates.second);
        }
    }
    
    // Add padding
    double padding = 50;
    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    
    if (rangeX < 1) rangeX = 1;
    if (rangeY < 1) rangeY = 1;
    
    // Calculate scale
    double scaleX = (width - 2 * padding) / rangeX;
    double scaleY = (height - 2 * padding) / rangeY;
    double scale = min(scaleX, scaleY);
    
    // Lambda to transform coordinates
    auto transformX = [&](double x) {
        return padding + (x - minX) * scale;
    };
    
    auto transformY = [&](double y) {
        // Flip Y axis (SVG has Y going down)
        return height - (padding + (y - minY) * scale);
    };
    
    // Lambda to get color for node type
    auto getNodeColor = [](NodeType type) {
        switch (type) {
            case NodeType::Charging: return "#22c55e";     // Green
            case NodeType::Pickup: return "#3b82f6";       // Blue
            case NodeType::Dropoff: return "#f59e0b";      // Orange
            case NodeType::Waypoint: return "#6b7280";     // Gray
            case NodeType::AFK: return "#8b5cf6";          // Purple
            case NodeType::ForbiddenCorner: return "#ef4444"; // Red
            case NodeType::ObstacleCorner: return "#dc2626";  // Dark Red
            default: return "#6b7280";
        }
    };
    
    // Lambda to get node type name
    auto getNodeTypeName = [](NodeType type) {
        switch (type) {
            case NodeType::Charging: return "Charging";
            case NodeType::Pickup: return "Pickup";
            case NodeType::Dropoff: return "Dropoff";
            case NodeType::Waypoint: return "Waypoint";
            case NodeType::AFK: return "AFK";
            case NodeType::ForbiddenCorner: return "Forbidden";
            case NodeType::ObstacleCorner: return "Obstacle";
            default: return "Unknown";
        }
    };
    
    // Write SVG header
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width 
         << "\" height=\"" << height << "\">\n";
    file << "<rect width=\"100%\" height=\"100%\" fill=\"#f9fafb\"/>\n";
    
    // Add title
    file << "<text x=\"" << width/2 << "\" y=\"30\" font-family=\"Arial\" font-size=\"20\" "
         << "text-anchor=\"middle\" font-weight=\"bold\">Graph Visualization</text>\n";
    
    // Draw edges first (so they appear behind nodes)
    file << "<g id=\"edges\">\n";
    set<pair<int, int>> drawnEdges; // Track drawn edges to avoid duplicates
    
    for (const auto& [nodeId, node] : nodes) {
        double x1 = transformX(node.coordinates.first);
        double y1 = transformY(node.coordinates.second);
        
        auto edgeIt = adjacencyList.find(nodeId);
        if (edgeIt != adjacencyList.end()) {
            for (const auto& edge : edgeIt->second) {
                int destId = edge.nodeDestinationId;
                
                // Only draw each edge once (for undirected graph)
                auto edgePair = make_pair(min(nodeId, destId), max(nodeId, destId));
                if (drawnEdges.count(edgePair)) continue;
                drawnEdges.insert(edgePair);
                
                const Node* destNode = getNode(destId);
                if (destNode) {
                    double x2 = transformX(destNode->coordinates.first);
                    double y2 = transformY(destNode->coordinates.second);
                    
                    file << "<line x1=\"" << x1 << "\" y1=\"" << y1 
                         << "\" x2=\"" << x2 << "\" y2=\"" << y2 
                         << "\" stroke=\"#94a3b8\" stroke-width=\"2\" opacity=\"0.6\"/>\n";
                    
                    // Add distance label in the middle of the edge
                    double midX = (x1 + x2) / 2;
                    double midY = (y1 + y2) / 2;
                    file << "<text x=\"" << midX << "\" y=\"" << midY 
                         << "\" font-family=\"Arial\" font-size=\"10\" fill=\"#64748b\" "
                         << "text-anchor=\"middle\">" << fixed << setprecision(1) 
                         << edge.distance << "</text>\n";
                }
            }
        }
    }
    file << "</g>\n";
    
    // Draw nodes
    file << "<g id=\"nodes\">\n";
    for (const auto& [nodeId, node] : nodes) {
        double x = transformX(node.coordinates.first);
        double y = transformY(node.coordinates.second);
        string color = getNodeColor(node.type);
        
        // Draw node circle
        file << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"20\" "
             << "fill=\"" << color << "\" stroke=\"#1e293b\" stroke-width=\"2\"/>\n";
        
        // Draw node ID
        file << "<text x=\"" << x << "\" y=\"" << y + 5 << "\" "
             << "font-family=\"Arial\" font-size=\"14\" font-weight=\"bold\" "
             << "fill=\"white\" text-anchor=\"middle\">" << nodeId << "</text>\n";
        
        // Draw node type and coordinates below
        file << "<text x=\"" << x << "\" y=\"" << y + 35 << "\" "
             << "font-family=\"Arial\" font-size=\"11\" fill=\"#1e293b\" "
             << "text-anchor=\"middle\">" << getNodeTypeName(node.type) << "</text>\n";
        
        file << "<text x=\"" << x << "\" y=\"" << y + 48 << "\" "
             << "font-family=\"Arial\" font-size=\"9\" fill=\"#64748b\" "
             << "text-anchor=\"middle\">(" << node.coordinates.first 
             << "," << node.coordinates.second << ")</text>\n";
    }
    file << "</g>\n";
    
    // Add legend
    int legendX = 10;
    int legendY = 60;
    file << "<g id=\"legend\">\n";
    file << "<rect x=\"" << legendX << "\" y=\"" << legendY 
         << "\" width=\"150\" height=\"200\" fill=\"white\" stroke=\"#cbd5e1\" "
         << "stroke-width=\"1\" opacity=\"0.9\" rx=\"5\"/>\n";
    
    file << "<text x=\"" << legendX + 10 << "\" y=\"" << legendY + 20 
         << "\" font-family=\"Arial\" font-size=\"12\" font-weight=\"bold\">Legend</text>\n";
    
    vector<pair<NodeType, string>> legendItems = {
        {NodeType::Charging, "Charging"},
        {NodeType::Pickup, "Pickup"},
        {NodeType::Dropoff, "Dropoff"},
        {NodeType::Waypoint, "Waypoint"},
        {NodeType::AFK, "AFK"},
        {NodeType::ForbiddenCorner, "Forbidden"}
    };
    
    int yOffset = legendY + 40;
    for (const auto& [type, name] : legendItems) {
        file << "<circle cx=\"" << legendX + 20 << "\" cy=\"" << yOffset 
             << "\" r=\"8\" fill=\"" << getNodeColor(type) << "\"/>\n";
        file << "<text x=\"" << legendX + 35 << "\" y=\"" << yOffset + 4 
             << "\" font-family=\"Arial\" font-size=\"11\">" << name << "</text>\n";
        yOffset += 25;
    }
    file << "</g>\n";
    
    file << "</svg>\n";
    file.close();
    
    cout << "SVG visualization saved to: " << filename << endl;
    return true;
}