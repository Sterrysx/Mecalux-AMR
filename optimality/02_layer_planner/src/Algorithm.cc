#include "Algorithm.hh"
#include <iostream>
using namespace std;

Algorithm::~Algorithm() {}

string Algorithm::getName() const {
    return "Base Algorithm";
}

string Algorithm::getDescription() const {
    return "Base class for planning algorithms";
}

void Algorithm::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots
) {
    cout << "Executing base Algorithm - should be overridden!" << endl;
}

