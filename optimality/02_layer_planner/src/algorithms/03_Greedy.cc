#include "../../include/algorithms/03_Greedy.hh"
#include <iostream>
using namespace std;

void Greedy::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots [[maybe_unused]],
    queue<Robot>& chargingRobots [[maybe_unused]],
    queue<Task>& pendingTasks,
    int totalRobots
) {
    cout << "Executing Greedy Algorithm..." << endl;
    cout << "Total robots: " << totalRobots << endl;
    cout << "Available robots: " << availableRobots.size() << endl;
    cout << "Pending tasks: " << pendingTasks.size() << endl;
    
    // TODO: Implement greedy algorithm
    // This would assign each task to the nearest available robot
    cout << "Greedy algorithm placeholder - to be implemented" << endl;
    
    cout << "Graph has " << graph.getNumVertices() << " vertices" << endl;
}

string Greedy::getName() const {
    return "Greedy";
}

string Greedy::getDescription() const {
    return "Assigns tasks to the nearest available robot (greedy heuristic)";
}
