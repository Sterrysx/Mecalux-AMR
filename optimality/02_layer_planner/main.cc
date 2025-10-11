#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "../01_layer_mapping/include/Graph.hh"
#include "include/Planifier.hh"
#include "include/Robot.hh"
using namespace std;

const int NUM_ROBOTS = 2;
const string DEFAULT_GRAPH = "../01_layer_mapping/tests/distributions/graph1.inp";
const int DEFAULT_ALGORITHM = 1;

void usage(const char* progname) {
    cerr << "Usage: " << progname << " [graph_number] [algorithm]" << endl;
    cerr << endl;
    cerr << "Arguments:" << endl;
    cerr << "  graph_number  : Number from 1-10 (default: 1)" << endl;
    cerr << "                  Loads from ../01_layer_mapping/tests/distributions/graphN.inp" << endl;
    cerr << "  algorithm     : Algorithm selection (default: 1)" << endl;
    cerr << "                  0 = Interactive mode" << endl;
    cerr << "                  1-3 = Direct algorithm selection" << endl;
    cerr << endl;
    cerr << "Examples:" << endl;
    cerr << "  " << progname << "              # Use graph1.inp with algorithm 1" << endl;
    cerr << "  " << progname << " 5           # Use graph5.inp with algorithm 1" << endl;
    cerr << "  " << progname << " 3 2         # Use graph3.inp with algorithm 2" << endl;
    exit(1);
}

queue<Task> read_tasks(const string& filename) {
    ifstream file(filename);
    queue<Task> QT;
    
    if (!file.is_open()) {
        cerr << "Error: Could not open task file " << filename << endl;
        return QT;
    }
    
    int id, origin, destination;
    while (file >> id >> origin >> destination) {
        Task task(id, origin, destination);
        QT.push(task);
    }
    
    file.close();
    return QT;
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    int graphNumber = 1;
    int algorithm = DEFAULT_ALGORITHM;
    
    if (argc > 3) {
        cerr << "Error: Too many arguments" << endl;
        usage(argv[0]);
    }
    
    if (argc >= 2) {
        graphNumber = atoi(argv[1]);
        if (graphNumber < 1 || graphNumber > 10) {
            cerr << "Error: graph_number must be between 1 and 10" << endl;
            usage(argv[0]);
        }
    }
    
    if (argc == 3) {
        algorithm = atoi(argv[2]);
        if (algorithm < 0 || algorithm > 4) {
            cerr << "Error: algorithm must be between 0 and 4" << endl;
            usage(argv[0]);
        }
    }
    
    // Construct graph file path
    string graphFile = "../01_layer_mapping/tests/distributions/graph" + 
                       to_string(graphNumber) + ".inp";
    
    cout << "=== Layer 02: Planner ===" << endl;
    cout << "Loading graph from: " << graphFile << endl;
    cout << "Algorithm: " << algorithm << endl;
    cout << "Number of robots: " << NUM_ROBOTS << endl;
    cout << endl;
    
    // Open and redirect stdin to the graph file
    ifstream inputFile(graphFile);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file " << graphFile << endl;
        return 1;
    }
    
    // Create graph using efficient loadFromStream method
    Graph G;
    G.loadFromStream(inputFile);
    
    inputFile.close();
    
    cout << "Graph loaded successfully with " << G.getNumVertices() << " vertices" << endl;
    

    // Create Task T with all the input from file case1.inp
    Task T;
    queue<Task> QT = read_tasks("tests/graph1/graph1_case1.inp");

    cout << "Loaded " << QT.size() << " tasks from file" << endl;

    // Create planner
    Planifier P(G, NUM_ROBOTS, QT);
    
    cout << "Planner created with " << P.getNumRobots() << " robots" << endl;
    cout << endl;
    
    // Run planning algorithm
    P.plan(algorithm);

    return 0;
}
