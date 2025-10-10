#ifndef PLANIFIER_H
#define PLANIFIER_H

#include <vector>
#include <unordered_map>
#include <set>
#include <utility>
#include <iostream>
#include <string>
#include <queue>
#include "Graph.hh"
#include "Robot.hh"
using namespace std;

//Where node is 
class Planifier {

public:
    // Constructors and Destructor
    Planifier();
    Planifier(const Graph& graph, int numRobots);
    ~Planifier();

    // Getters
    int getNumRobots() const;
    int getAvailableRobots() const;
    int getBusyRobots() const;
    int getChargingRobots() const;
    Graph getGraph() const;

    // Setters
    void setGraph(const Graph& graph);
    void setNumRobots(int numRobots);
    void setAvailableRobots(int num);
    void setBusyRobots(int num);
    void setChargingRobots(int num);

    // Planning methods
    void plan(int algorithm = 0);  // 0 = interactive, 1-4 = direct selection


private:
    Graph G;
    queue<Robot> availableRobots;
    queue<Robot> busyRobots;
    queue<Robot> chargingRobots;
    const int numRobots;
};

#endif // PLANIFIER_H