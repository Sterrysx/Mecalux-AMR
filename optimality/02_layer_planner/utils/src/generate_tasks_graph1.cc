#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>
#include <ctime>
using namespace std;

// Node type identifiers for graph1
const vector<int> PICKUP_NODES = {2, 3, 4, 5};      // P nodes
const vector<int> DROPOFF_NODES = {11, 12, 13};     // D nodes
const int NUM_NODES = 17;  // Total nodes in graph1

void printUsage(const char* progname) {
    cerr << "Usage: " << progname << " <num_cases> <packets_per_case> [seed]" << endl;
    cerr << "   or: " << progname << " -i <num_cases> <start> <increment> [seed]" << endl;
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
    cerr << "Output: Generates files in ../../tests/graph1/" << endl;
    cerr << "        Format: graph1_caseN.inp (N = 1 to num_cases)" << endl;
    cerr << endl;
    cerr << "Examples:" << endl;
    cerr << "  " << progname << " 10 15           # 10 cases with 15 packets each" << endl;
    cerr << "  " << progname << " 5 20 42         # 5 cases with 20 packets, seed=42" << endl;
    cerr << "  " << progname << " -i 10 10 10     # 10 cases: 10, 20, 30, ..., 100 packets" << endl;
    cerr << "  " << progname << " -i 5 5 5 999    # 5 cases: 5, 10, 15, 20, 25 packets, seed=999" << endl;
}

int main(int argc, char* argv[]) {
    bool incrementalMode = false;
    int argOffset = 1;
    
    // Check for incremental mode flag
    if (argc > 1 && string(argv[1]) == "-i") {
        incrementalMode = true;
        argOffset = 2;
    }
    
    int requiredArgs = incrementalMode ? 4 : 3;
    int maxArgs = incrementalMode ? 5 : 4;
    
    if (argc < argOffset + 2 || argc > argOffset + maxArgs - 1) {
        printUsage(argv[0]);
        return 1;
    }
    
    int numCases = atoi(argv[argOffset]);
    int packetsPerCase = 0;
    int startPackets = 0;
    int increment = 0;
    unsigned int seed;
    
    if (incrementalMode) {
        // Mode: -i <num_cases> <start> <increment> [seed]
        if (argc < argOffset + 3) {
            printUsage(argv[0]);
            return 1;
        }
        startPackets = atoi(argv[argOffset + 1]);
        increment = atoi(argv[argOffset + 2]);
        seed = (argc == argOffset + 4) ? atoi(argv[argOffset + 3]) : time(nullptr);
        
        if (numCases <= 0 || startPackets <= 0 || increment < 0) {
            cerr << "Error: num_cases and start must be positive, increment must be non-negative" << endl;
            return 1;
        }
    } else {
        // Mode: <num_cases> <packets_per_case> [seed]
        packetsPerCase = atoi(argv[argOffset + 1]);
        seed = (argc == argOffset + 3) ? atoi(argv[argOffset + 2]) : time(nullptr);
        
        if (numCases <= 0 || packetsPerCase <= 0) {
            cerr << "Error: num_cases and packets_per_case must be positive integers" << endl;
            return 1;
        }
    }
    
    cout << "=== Packet Generator for Graph1 ===" << endl;
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
        cout << "Packets per case: " << packetsPerCase << endl;
    }
    cout << "Random seed: " << seed << endl;
    cout << "Output directory: ../../tests/graph1/" << endl;
    cout << endl;
    
    // Create output directory (Linux/Unix command)
    system("mkdir -p ../../tests/graph1");
    
    // Initialize random number generator
    mt19937 rng(seed);
    uniform_int_distribution<int> pickupDist(0, PICKUP_NODES.size() - 1);
    uniform_int_distribution<int> dropoffDist(0, DROPOFF_NODES.size() - 1);
    
    // Generate test cases
    for (int caseNum = 1; caseNum <= numCases; caseNum++) {
        int currentPackets = incrementalMode ? (startPackets + (caseNum - 1) * increment) : packetsPerCase;
        
        string filename = "../../tests/graph1/graph1_case" + to_string(caseNum) + ".inp";
        ofstream outFile(filename);
        
        if (!outFile.is_open()) {
            cerr << "Error: Could not create file " << filename << endl;
            return 1;
        }
        
        // Write number of packets
        outFile << currentPackets << endl;
        
        // Generate random packets
        for (int packetId = 0; packetId < currentPackets; packetId++) {
            int pickupIdx = pickupDist(rng);
            int dropoffIdx = dropoffDist(rng);
            
            int pickupNode = PICKUP_NODES[pickupIdx];
            int dropoffNode = DROPOFF_NODES[dropoffIdx];
            
            outFile << packetId << " " << pickupNode << " " << dropoffNode << endl;
        }
        
        outFile.close();
        cout << "Generated: " << filename << " (" << currentPackets << " packets)" << endl;
    }
    
    cout << endl;
    cout << "Successfully generated " << numCases << " test cases!" << endl;
    cout << endl;
    cout << "Pickup nodes used: ";
    for (int node : PICKUP_NODES) cout << node << " ";
    cout << endl;
    cout << "Dropoff nodes used: ";
    for (int node : DROPOFF_NODES) cout << node << " ";
    cout << endl;
    
    return 0;
}
