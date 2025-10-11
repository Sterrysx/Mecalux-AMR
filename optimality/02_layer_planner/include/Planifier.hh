#ifndef PLANIFIER_H
#define PLANIFIER_H

#include <vector>
#include <unordered_map>
#include <set>
#include <utility>
#include <iostream>
#include <string>
#include <queue>
#include <memory>
#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"
#include "Algorithm.hh"
using namespace std;

//Where node is 
class Planifier {

public:
    // Constructors and Destructor
    Planifier();
    Planifier(const Graph& graph, int numRobots, queue<Task> tasks);
    ~Planifier();

    // Getters
    int getNumRobots() const;
    int getAvailableRobots() const;
    int getBusyRobots() const;
    int getChargingRobots() const;
    Graph getGraph() const;
    queue<Task> getPendingTasks() const;

    // Setters
    void setGraph(const Graph& graph);
    void setNumRobots(int numRobots);
    void setAvailableRobots(int num);
    void setBusyRobots(int num);
    void setChargingRobots(int num);
    void setPendingTasks(int num);

    // Strategy Pattern: Set the planning algorithm
    void setAlgorithm(unique_ptr<Algorithm> algorithm);

    // Planning methods
    void plan(int algorithmChoice = 0);  // 0 = interactive, 1-4 = direct selection
    void executePlan();                  // Execute the currently set algorithm


private:
    Graph G;
    queue<Robot> availableRobots;
    queue<Robot> busyRobots;
    queue<Robot> chargingRobots;
    const int totalRobots;
    queue<Task> pendingTasks;
    
    // Strategy Pattern: Current planning algorithm
    unique_ptr<Algorithm> currentAlgorithm;
};

#endif // PLANIFIER_H