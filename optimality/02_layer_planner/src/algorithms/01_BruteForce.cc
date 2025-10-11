#include "../include/algorithms/BruteForce.hh"
#include <iostream>
using namespace std;

void BruteForce::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots
) {
    cout << "Executing Brute Force Algorithm..." << endl;
    cout << "Total robots: " << totalRobots << endl;
    cout << "Available robots: " << availableRobots.size() << endl;
    cout << "Pending tasks: " << pendingTasks.size() << endl;
    
    // TODO: Implement actual brute-force algorithm
    // This would explore all possible task assignments and select the optimal one
    cout << "Brute force algorithm placeholder - to be implemented" << endl;
    
    // Example: Just print graph info for now
    cout << "Graph has " << graph.getNumVertices() << " vertices" << endl;
}

string BruteForce::getName() const {
    return "Brute Force";
}

string BruteForce::getDescription() const {
    return "Explores all possible task assignments to find the optimal solution";
}
