#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>
#include <ctime>
#include <cstdlib>
#include <map>
using namespace std;

/**
 * @brief Graph configuration structure
 * Defines pickup nodes, dropoff nodes, and charging nodes for each graph type
 */
struct GraphConfig {
    vector<int> pickupNodes;
    vector<int> dropoffNodes;
    vector<int> chargingNodes;
    int totalNodes;
    string description;
};

/**
 * @brief Configuration for all 10 graph types
 * Based on analysis of graph*.inp files in 01_layer_mapping/tests/distributions/
 */
const map<int, GraphConfig> GRAPH_CONFIGS = {
    // Graph 1: 17 nodes
    {1, {
        {2, 3, 4, 5},           // Pickup nodes (P)
        {11, 12, 13},           // Dropoff nodes (D)
        {0, 1},                 // Charging nodes (C)
        17,
        "Graph 1: 4 pickup nodes, 3 dropoff nodes, 2 charging stations"
    }},
    
    // Graph 2: 12 nodes
    {2, {
        {2, 3, 4, 5},           // Pickup nodes (P)
        {6, 7, 8, 9},           // Dropoff nodes (D)
        {0, 1},                 // Charging nodes (C)
        12,
        "Graph 2: 4 pickup nodes, 4 dropoff nodes, 2 charging stations"
    }},
    
    // Graph 3: 20 nodes
    {3, {
        {3, 4, 5, 6},           // Pickup nodes (P)
        {7, 8, 9, 10},          // Dropoff nodes (D)
        {0, 1, 2},              // Charging nodes (C)
        20,
        "Graph 3: 4 pickup nodes, 4 dropoff nodes, 3 charging stations"
    }},
    
    // Graph 4: 22 nodes
    {4, {
        {3, 4, 5, 6, 7},        // Pickup nodes (P)
        {8, 9, 10, 11, 12},     // Dropoff nodes (D)
        {0, 1, 2},              // Charging nodes (C)
        22,
        "Graph 4: 5 pickup nodes, 5 dropoff nodes, 3 charging stations"
    }},
    
    // Graph 5: 28 nodes
    {5, {
        {3, 4, 5, 6, 7},        // Pickup nodes (P)
        {8, 9, 10, 11, 12, 13, 14}, // Dropoff nodes (D)
        {0, 1, 2},              // Charging nodes (C)
        28,
        "Graph 5: 5 pickup nodes, 7 dropoff nodes, 3 charging stations"
    }},
    
    // Graph 6: 32 nodes
    {6, {
        {3, 4, 5, 6, 7, 8},     // Pickup nodes (P)
        {9, 10, 11, 12, 13, 14, 15, 16}, // Dropoff nodes (D)
        {0, 1, 2},              // Charging nodes (C)
        32,
        "Graph 6: 6 pickup nodes, 8 dropoff nodes, 3 charging stations"
    }},
    
    // Graph 7: 38 nodes
    {7, {
        {5, 6, 7, 8, 9, 10, 11}, // Pickup nodes (P)
        {12, 13, 14, 15, 16, 17, 18}, // Dropoff nodes (D)
        {0, 1, 2, 3, 4},        // Charging nodes (C)
        38,
        "Graph 7: 7 pickup nodes, 7 dropoff nodes, 5 charging stations"
    }},
    
    // Graph 8: 42 nodes
    {8, {
        {5, 6, 7, 8, 9, 10, 11, 12}, // Pickup nodes (P)
        {13, 14, 15, 16, 17, 18, 19, 20}, // Dropoff nodes (D)
        {0, 1, 2, 3, 4},        // Charging nodes (C)
        42,
        "Graph 8: 8 pickup nodes, 8 dropoff nodes, 5 charging stations"
    }},
    
    // Graph 9: 46 nodes
    {9, {
        {5, 6, 7, 8, 9, 10, 11, 12, 13}, // Pickup nodes (P)
        {14, 15, 16, 17, 18, 19, 20, 21, 22}, // Dropoff nodes (D)
        {0, 1, 2, 3, 4},        // Charging nodes (C)
        46,
        "Graph 9: 9 pickup nodes, 9 dropoff nodes, 5 charging stations"
    }},
    
    // Graph 10: 50 nodes
    {10, {
        {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}, // Pickup nodes (P)
        {15, 16, 17, 18, 19, 20, 21, 22, 23, 24}, // Dropoff nodes (D)
        {0, 1, 2, 3, 4},        // Charging nodes (C)
        50,
        "Graph 10: 10 pickup nodes, 10 dropoff nodes, 5 charging stations"
    }}
};

/**
 * @brief Get output directory based on graph ID
 */
string getOutputDirectory(int graphId) {
    const char* rootEnv = getenv("MECALUX_ROOT");
    string base;
    
    if (rootEnv != nullptr) {
        base = string(rootEnv) + "/optimality/02_layer_planner/tests/graph" + to_string(graphId);
    } else {
        // When run from utils/, we need ../tests/graphN
        // When run from 02_layer_planner/, we need tests/graphN
        // Try to detect and use appropriate path
        base = "../tests/graph" + to_string(graphId);
    }
    
    return base;
}

/**
 * @brief Print usage information
 */
void printUsage(const char* progname) {
    cerr << "Usage: " << progname << " <graphID> <num_cases> <packets_per_case> [seed]" << endl;
    cerr << "   or: " << progname << " <graphID> -i <num_cases> <start> <increment> [seed]" << endl;
    cerr << endl;
    cerr << "Arguments:" << endl;
    cerr << "  graphID          : Graph number (1-10)" << endl;
    cerr << endl;
    cerr << "Mode 1 (fixed):" << endl;
    cerr << "  num_cases        : Number of test cases to generate (e.g., 10)" << endl;
    cerr << "  packets_per_case : Number of packets per test case (e.g., 15)" << endl;
    cerr << "  seed             : Random seed (optional, default: current time)" << endl;
    cerr << endl;
    cerr << "Mode 2 (incremental):" << endl;
    cerr << "  -i               : Incremental mode flag" << endl;
    cerr << "  num_cases        : Number of test cases to generate (e.g., 10)" << endl;
    cerr << "  start            : Starting number of packets (e.g., 10)" << endl;
    cerr << "  increment        : Increment per case (e.g., 10)" << endl;
    cerr << "  seed             : Random seed (optional, default: current time)" << endl;
    cerr << endl;
    cerr << "Output: Generates files in tests/graph<ID>/" << endl;
    cerr << "        Format: M_tasks.inp (M = number of packets)" << endl;
    cerr << "        Uses MECALUX_ROOT env var if set, otherwise relative path" << endl;
    cerr << endl;
    cerr << "Examples:" << endl;
    cerr << "  " << progname << " 1 10 15           # Graph1: 10 files with 15 packets each" << endl;
    cerr << "  " << progname << " 2 5 20 42         # Graph2: 5 files with 20 packets, seed=42" << endl;
    cerr << "  " << progname << " 3 -i 10 10 10     # Graph3: 10_tasks.inp, 20_tasks.inp, ..., 100_tasks.inp" << endl;
    cerr << "  " << progname << " 5 -i 5 5 5 999    # Graph5: 5_tasks.inp, 10_tasks.inp, ..., 25_tasks.inp, seed=999" << endl;
    cerr << endl;
    cerr << "Available Graphs:" << endl;
    for (const auto& pair : GRAPH_CONFIGS) {
        cerr << "  Graph " << pair.first << ": " << pair.second.description << endl;
    }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Minimum 3 arguments: progname graphID num_cases
    if (argc < 4) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse graph ID
    int graphId = atoi(argv[1]);
    if (graphId < 1 || graphId > 10) {
        cerr << "Error: graphID must be between 1 and 10" << endl;
        cerr << endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Check if graph config exists
    auto configIt = GRAPH_CONFIGS.find(graphId);
    if (configIt == GRAPH_CONFIGS.end()) {
        cerr << "Error: Graph " << graphId << " configuration not found" << endl;
        return 1;
    }
    
    const GraphConfig& config = configIt->second;
    
    // Check for incremental mode
    bool incrementalMode = false;
    int argOffset = 2;
    
    if (argc > 2 && string(argv[2]) == "-i") {
        incrementalMode = true;
        argOffset = 3;
    }
    
    // Parse remaining arguments based on mode
    int numCases = 0;
    int packetsPerCase = 0;
    int startPackets = 0;
    int increment = 0;
    unsigned int seed;
    
    if (incrementalMode) {
        // Mode: <graphID> -i <num_cases> <start> <increment> [seed]
        if (argc < argOffset + 3) {
            cerr << "Error: Incremental mode requires: <graphID> -i <num_cases> <start> <increment> [seed]" << endl;
            printUsage(argv[0]);
            return 1;
        }
        
        numCases = atoi(argv[argOffset]);
        startPackets = atoi(argv[argOffset + 1]);
        increment = atoi(argv[argOffset + 2]);
        seed = (argc > argOffset + 3) ? atoi(argv[argOffset + 3]) : time(nullptr);
        
        if (numCases <= 0 || startPackets <= 0 || increment < 0) {
            cerr << "Error: num_cases and start must be positive, increment must be non-negative" << endl;
            return 1;
        }
    } else {
        // Mode: <graphID> <num_cases> <packets_per_case> [seed]
        if (argc < argOffset + 2) {
            cerr << "Error: Fixed mode requires: <graphID> <num_cases> <packets_per_case> [seed]" << endl;
            printUsage(argv[0]);
            return 1;
        }
        
        numCases = atoi(argv[argOffset]);
        packetsPerCase = atoi(argv[argOffset + 1]);
        seed = (argc > argOffset + 2) ? atoi(argv[argOffset + 2]) : time(nullptr);
        
        if (numCases <= 0 || packetsPerCase <= 0) {
            cerr << "Error: num_cases and packets_per_case must be positive integers" << endl;
            return 1;
        }
    }
    
    // Print configuration
    cout << "=== Universal Task Generator ===" << endl;
    cout << "Graph ID: " << graphId << endl;
    cout << "Description: " << config.description << endl;
    cout << "Generating " << numCases << " test cases" << endl;
    
    if (incrementalMode) {
        cout << "Mode: Incremental (start=" << startPackets << ", increment=" << increment << ")" << endl;
        cout << "Packets per case: " << startPackets;
        for (int i = 1; i < min(numCases, 5); i++) {
            cout << ", " << (startPackets + i * increment);
        }
        if (numCases > 5) cout << ", ...";
        cout << endl;
    } else {
        cout << "Mode: Fixed (" << packetsPerCase << " packets per case)" << endl;
        cout << "Packets per case: " << packetsPerCase << endl;
    }
    
    cout << "Random seed: " << seed << endl;
    
    string outputDir = getOutputDirectory(graphId);
    cout << "Output directory: " << outputDir << "/" << endl;
    cout << endl;
    
    // Create output directory
    string mkdirCmd = "mkdir -p " + outputDir;
    system(mkdirCmd.c_str());
    
    // Initialize random number generator
    mt19937 rng(seed);
    uniform_int_distribution<int> pickupDist(0, config.pickupNodes.size() - 1);
    uniform_int_distribution<int> dropoffDist(0, config.dropoffNodes.size() - 1);
    
    // Generate test cases
    for (int caseNum = 1; caseNum <= numCases; caseNum++) {
        int currentPackets = incrementalMode ? (startPackets + (caseNum - 1) * increment) : packetsPerCase;
        
        string filename = outputDir + "/" + to_string(currentPackets) + "_tasks.inp";
        ofstream outFile(filename);
        
        if (!outFile.is_open()) {
            cerr << "Error: Could not create file " << filename << endl;
            return 1;
        }
        
        // Write number of packets (first line is the total count)
        outFile << currentPackets << endl;
        
        // Generate random packets
        for (int packetId = 0; packetId < currentPackets; packetId++) {
            int pickupIdx = pickupDist(rng);
            int dropoffIdx = dropoffDist(rng);
            
            int pickupNode = config.pickupNodes[pickupIdx];
            int dropoffNode = config.dropoffNodes[dropoffIdx];
            
            outFile << packetId << " " << pickupNode << " " << dropoffNode << endl;
        }
        
        outFile.close();
        cout << "Generated: " << filename << " (" << currentPackets << " packets)" << endl;
    }
    
    cout << endl;
    cout << "Successfully generated " << numCases << " test cases for Graph " << graphId << "!" << endl;
    cout << endl;
    cout << "Configuration for Graph " << graphId << ":" << endl;
    cout << "  Pickup nodes:   ";
    for (int node : config.pickupNodes) cout << node << " ";
    cout << endl;
    cout << "  Dropoff nodes:  ";
    for (int node : config.dropoffNodes) cout << node << " ";
    cout << endl;
    cout << "  Charging nodes: ";
    for (int node : config.chargingNodes) cout << node << " ";
    cout << endl;
    cout << "  Total nodes:    " << config.totalNodes << endl;
    
    return 0;
}
