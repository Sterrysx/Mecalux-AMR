# Graph Visualization System - Layer 1: Graph Mapping

This directory contains the graph mapping layer for the AMR (Autonomous Mobile Robot) warehouse navigation system.

## 📁 Directory Structure

```
01_layer_mapping/
├── distributions/          # Input graph files
│   ├── graph1.inp         # Complex warehouse layout (17 nodes, 27 edges)
│   ├── graph2.inp         # Symmetric distribution (12 nodes, 15 edges)
│   ├── graph3.inp         # Large scale network (20 nodes, 28 edges)
│   ├── graph4.inp         # Medium warehouse (22 nodes, 35 edges)
│   ├── graph5.inp         # Dense distribution (28 nodes, 48 edges)
│   ├── graph6.inp         # Expanded warehouse (32 nodes, 56 edges)
│   ├── graph7.inp         # Multi-level complex (38 nodes, 68 edges)
│   ├── graph8.inp         # Large facility (42 nodes, 78 edges)
│   ├── graph9.inp         # Mega distribution (46 nodes, 88 edges)
│   └── graph10.inp        # Maximum complexity (50 nodes, 95 edges)
├── include/               # Header files
│   └── Graph.hh          # Graph class definition
├── src/                   # Implementation files
│   └── Graph.cc          # Graph class implementation
├── output/                # Generated visualizations
│   ├── graph1_visualization.svg
│   ├── graph2_visualization.svg
│   ├── graph3_visualization.svg
│   ├── graph4_visualization.svg
│   ├── graph5_visualization.svg
│   ├── graph6_visualization.svg
│   ├── graph7_visualization.svg
│   ├── graph8_visualization.svg
│   ├── graph9_visualization.svg
│   └── graph10_visualization.svg
├── main.cpp              # Main program
├── Makefile              # Build configuration
├── viewer.html           # Interactive web viewer
└── README.md             # This file
```

## 🚀 Quick Start

### Build and Run

```bash
# Compile the program
make

# Run and generate all visualizations
./graph_demo

# Clean build files
make clean
```

### View Visualizations

**Option 1: Web Viewer (Recommended)**
```bash
# Open the interactive viewer in your browser
firefox viewer.html
# or
explorer.exe viewer.html
```

**Option 2: Individual SVG Files**
```bash
# Open individual graphs
firefox output/graph1_visualization.svg
firefox output/graph2_visualization.svg
firefox output/graph3_visualization.svg
```

## 📊 Graph Input Format

Each `.inp` file follows this format:

```
<num_vertices> <num_edges> <zone_width> <zone_height>

<node_id> <x> <y> <type>
<node_id> <x> <y> <type>
...

<from_node> <to_node>
<from_node> <to_node>
...
```

### Node Types
- `C` - Charging station (Green)
- `P` - Pickup point (Blue)
- `D` - Dropoff point (Orange)
- `W` - Waypoint (Gray)
- `A` - AFK zone (Purple)
- `F` - Forbidden corner (Red)
- `O` - Obstacle corner (Dark Red)

### Example
```
17 27 20 30

0 0 0 C
1 0 2 C
...

0 14
1 14
...
```

## 🎨 Visualization Features

The generated SVG files include:
- ✅ Color-coded nodes by type
- ✅ Node IDs clearly labeled
- ✅ Edge distances displayed
- ✅ Coordinate information
- ✅ Interactive legend
- ✅ Automatic scaling
- ✅ Professional styling

## 🔧 Graph Class API

### Key Methods

```cpp
// Load graph from file
bool loadFromFile(const std::string& filename);

// Add nodes and edges
int addNode(int nodeId, NodeType type, double x, double y);
bool addEdge(int fromNodeId, int toNodeId, double distance = 0.0, double speed = 1.6);

// Query graph
const Node* getNode(int nodeId) const;
const std::vector<Edge>& getEdges(int nodeId) const;
int getNumVertices() const;

// Visualization
bool generateSVG(const std::string& filename, int width = 800, int height = 600) const;
void printGraph() const;
```

## 📝 Creating New Graphs

1. Create a new `.inp` file in `distributions/`
2. Follow the format specified above
3. Add the graph to `main.cpp` in the graphs vector
4. Rebuild and run

Example:
```cpp
{"distributions/graph4.inp", "output/graph4_visualization.svg"}
```

## 🛠️ Dependencies

- C++17 compiler (g++, clang++)
- Standard library only (no external dependencies)
- Web browser for viewing SVG files

## 📈 Graph Statistics

| Graph | Nodes | Edges | Zone Size | Description |
|-------|-------|-------|-----------|-------------|
| Graph 1 | 17 | 27 | 20×30 | Complex warehouse with multiple zones |
| Graph 2 | 12 | 15 | 25×20 | Symmetric distribution network |
| Graph 3 | 20 | 28 | 30×25 | Large scale multi-level layout |
| Graph 4 | 22 | 35 | 35×28 | Medium warehouse layout |
| Graph 5 | 28 | 48 | 40×35 | Dense distribution network |
| Graph 6 | 32 | 56 | 45×40 | Expanded warehouse facility |
| Graph 7 | 38 | 68 | 50×45 | Multi-level complex |
| Graph 8 | 42 | 78 | 55×50 | Large scale facility |
| Graph 9 | 46 | 88 | 60×52 | Mega distribution center |
| Graph 10 | 50 | 95 | 65×55 | Maximum complexity warehouse |

**Total Dataset:** 307 nodes, 538 edges across 10 diverse warehouse configurations

## 🐛 Troubleshooting

### Compilation Issues
```bash
# Ensure you have g++ installed
g++ --version

# Clean and rebuild
make clean
make
```

### Visualization Not Loading
- Ensure the program ran successfully
- Check that `output/` directory exists
- Verify SVG files were generated
- Try opening SVG files directly in browser

## 📄 License

Part of the Mecalux-AMR warehouse automation project.

## 👥 Author

Developed for warehouse AMR navigation and path planning optimization.
