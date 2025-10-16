#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include "SimulationController.hh"
#include "Graph.hh"
#include "Task.hh"

using namespace std;

/**
 * @brief Load a graph from file
 * 
 * @param graphId Graph identifier (1-10)
 * @return Graph object
 */
Graph loadGraph(int graphId) {
    string graphPath = "../01_layer_mapping/tests/distributions/graph" + to_string(graphId) + ".inp";
    
    ifstream graphFile(graphPath);
    if (!graphFile.is_open()) {
        cerr << "Error: Could not open graph file: " << graphPath << endl;
        exit(1);
    }
    
    Graph graph;
    graph.loadFromStream(graphFile);
    graphFile.close();
    
    return graph;
}

/**
 * @brief Load tasks from a test file
 * 
 * @param graphId Graph identifier (1-10)
 * @param numTasks Number of tasks to load (e.g., 10 for "10_tasks.inp")
 * @return Queue of tasks
 */
queue<Task> loadTasks(int graphId, int numTasks) {
    string taskPath = "../02_layer_planner/test_data/graph" + to_string(graphId) + 
                      "/" + to_string(numTasks) + "_tasks.inp";
    
    ifstream taskFile(taskPath);
    if (!taskFile.is_open()) {
        cerr << "Warning: Could not open task file: " << taskPath << endl;
        cerr << "Starting with empty task queue." << endl;
        return queue<Task>();
    }
    
    queue<Task> tasks;
    int taskCount;
    
    // First line contains the number of tasks
    if (!(taskFile >> taskCount)) {
        cerr << "Warning: Invalid task file format" << endl;
        taskFile.close();
        return queue<Task>();
    }
    
    // Read each task: taskId origin destination
    for (int i = 0; i < taskCount; i++) {
        int taskId, origin, destination;
        if (taskFile >> taskId >> origin >> destination) {
            tasks.push(Task(taskId, origin, destination));
        }
    }
    
    taskFile.close();
    return tasks;
}

/**
 * @brief Print usage information
 */
void printUsage(const char* programName) {
    cout << "=== Real-Time Warehouse Simulation ===" << endl;
    cout << endl;
    cout << "Usage: " << programName << " <graphId> [numRobots] [numTasks]" << endl;
    cout << endl;
    cout << "Arguments:" << endl;
    cout << "  graphId    - Graph identifier (1-10)" << endl;
    cout << "  numRobots  - Number of robots (default: 5)" << endl;
    cout << "  numTasks   - Number of initial tasks to load (e.g., 10, 12, 15)" << endl;
    cout << "               Loads from test_data/graph<N>/<numTasks>_tasks.inp" << endl;
    cout << "               (default: none - start with empty task queue)" << endl;
    cout << endl;
    cout << "Examples:" << endl;
    cout << "  " << programName << " 1                # Graph 1, 5 robots, no initial tasks" << endl;
    cout << "  " << programName << " 3 10             # Graph 3, 10 robots, no initial tasks" << endl;
    cout << "  " << programName << " 1 5 10           # Graph 1, 5 robots, load 10 tasks" << endl;
    cout << "  " << programName << " 1 8 15           # Graph 1, 8 robots, load 15 tasks" << endl;
    cout << endl;
    cout << "During simulation, type 'help' for available commands." << endl;
    cout << "=======================================" << endl;
}

int main(int argc, char* argv[]) {
    // Check for help flag
    if (argc > 1 && (string(argv[1]) == "-h" || string(argv[1]) == "--help" || string(argv[1]) == "help")) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Parse command line arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    int graphId = atoi(argv[1]);
    int numRobots = (argc >= 3) ? atoi(argv[2]) : 5;
    int numTasks = (argc >= 4) ? atoi(argv[3]) : -1;
    
    // Validate inputs
    if (graphId < 1 || graphId > 10) {
        cerr << "Error: graphId must be between 1 and 10" << endl;
        return 1;
    }
    
    if (numRobots < 1 || numRobots > 100) {
        cerr << "Error: numRobots must be between 1 and 100" << endl;
        return 1;
    }
    
    // Load graph
    cout << "Loading graph " << graphId << "..." << endl;
    Graph graph = loadGraph(graphId);
    cout << "Graph loaded: " << graph.getNumVertices() << " nodes" << endl;
    
    // Load initial tasks (if specified)
    queue<Task> initialTasks;
    if (numTasks > 0) {
        cout << "Loading " << numTasks << " tasks..." << endl;
        initialTasks = loadTasks(graphId, numTasks);
        cout << "Loaded " << initialTasks.size() << " tasks" << endl;
    } else {
        cout << "Starting with empty task queue" << endl;
    }
    
    cout << endl;
    
    // Create and start simulation controller
    SimulationController controller(graph, numRobots, initialTasks);
    
    // Start the simulation (this blocks until user quits)
    controller.start();
    
    return 0;
}
