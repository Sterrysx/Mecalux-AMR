#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include "../01_layer_mapping/include/Graph.hh"
#include "include/Planifier.hh"
#include "include/Robot.hh"
using namespace std;

bool fileExists(const string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void usage(const char* progname) {
    cerr << "Usage: " << progname << " [algorithmID] [graphID] [taskID] [numRobots]" << endl;
    cerr << "       " << progname << " -h | --help" << endl;
    cerr << endl;
    cerr << "Arguments:" << endl;
    cerr << "  algorithmID   : Algorithm selection (default: 1)" << endl;
    cerr << "                  1 = Brute Force" << endl;
    cerr << "                  2 = Dynamic Programming" << endl;
    cerr << "                  3 = Greedy" << endl;
    cerr << "  graphID       : Graph number from 1-10 (default: 1)" << endl;
    cerr << "                  Loads from ../01_layer_mapping/tests/distributions/graphN.inp" << endl;
    cerr << "  taskID        : Task case number from 1-10 (default: 1)" << endl;
    cerr << "                  Loads from tests/graphN/graphN_caseM.inp" << endl;
    cerr << "  numRobots     : Number of robots (default: 2, range: 1-10)" << endl;
    cerr << endl;
    cerr << "Options:" << endl;
    cerr << "  -h, --help    : Show this help message" << endl;
    cerr << endl;
    cerr << "Examples:" << endl;
    cerr << "  " << progname << "                # Use algorithm 1, graph1, case1, 2 robots" << endl;
    cerr << "  " << progname << " 2             # Use algorithm 2, graph1, case1, 2 robots" << endl;
    cerr << "  " << progname << " 1 5           # Use algorithm 1, graph5, case1, 2 robots" << endl;
    cerr << "  " << progname << " 3 2 5         # Use algorithm 3, graph2, case5, 2 robots" << endl;
    cerr << "  " << progname << " 1 1 1 4       # Use algorithm 1, graph1, case1, 4 robots" << endl;
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
    // Check for help flag
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
    }
    
    // Parse command-line arguments with defaults
    int algorithm = 1;    // Default: Brute Force
    int graphID = 1;      // Default: graph1
    int taskID = 1;       // Default: case1
    int numRobots = 2;    // Default: 2 robots
    
    if (argc > 5) {
        cerr << "Error: Too many arguments" << endl;
        usage(argv[0]);
    }
    
    // Parse algorithmID
    if (argc >= 2) {
        algorithm = atoi(argv[1]);
        if (algorithm < 1 || algorithm > 3) {
            cerr << "Error: algorithmID must be between 1 and 3" << endl;
            usage(argv[0]);
        }
    }
    
    // Parse graphID
    if (argc >= 3) {
        graphID = atoi(argv[2]);
        if (graphID < 1 || graphID > 10) {
            cerr << "Error: graphID must be between 1 and 10" << endl;
            usage(argv[0]);
        }
    }
    
    // Parse taskID
    if (argc >= 4) {
        taskID = atoi(argv[3]);
        if (taskID < 1 || taskID > 10) {
            cerr << "Error: taskID must be between 1 and 10" << endl;
            usage(argv[0]);
        }
    }
    
    // Parse numRobots
    if (argc == 5) {
        numRobots = atoi(argv[4]);
        if (numRobots < 1 || numRobots > 10) {
            cerr << "Error: numRobots must be between 1 and 10" << endl;
            usage(argv[0]);
        }
    }
    
    // Construct file paths
    string graphFile = "../01_layer_mapping/tests/distributions/graph" + 
                       to_string(graphID) + ".inp";
    string taskFile = "tests/graph" + to_string(graphID) + "/graph" + 
                      to_string(graphID) + "_case" + to_string(taskID) + ".inp";
    
    // Verify files exist
    if (!fileExists(graphFile)) {
        cerr << "Error: Graph file does not exist: " << graphFile << endl;
        cerr << "Available graphs are graph1.inp through graph10.inp" << endl;
        return 1;
    }
    
    if (!fileExists(taskFile)) {
        cerr << "Error: Task file does not exist: " << taskFile << endl;
        cerr << "Available task files for graph" << graphID << " are:" << endl;
        cerr << "  tests/graph" << graphID << "/graph" << graphID << "_case1.inp through _case10.inp" << endl;
        return 1;
    }
    
    cout << "=== Layer 02: Planner ===" << endl;
    cout << "Algorithm ID: " << algorithm << endl;
    cout << "Graph file:   " << graphFile << endl;
    cout << "Task file:    " << taskFile << endl;
    cout << "Number of robots: " << numRobots << endl;
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
    
    // Load tasks from the specified task file
    queue<Task> QT = read_tasks(taskFile);
    
    if (QT.empty()) {
        cerr << "Error: No tasks loaded from " << taskFile << endl;
        return 1;
    }

    cout << "Loaded " << QT.size() << " tasks from file" << endl;

    // Create planner
    Planifier P(G, numRobots, QT);
    
    cout << "Planner created with " << P.getNumRobots() << " robots" << endl;
    cout << endl;
    
    // Run planning algorithm
    P.plan(algorithm);

    return 0;
}
