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
#include <optional>
#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"
#include "Algorithm.hh"

//Where node is 
class Planifier {

public:
    // Constructors and Destructor

    struct ComparisonReport {
        std::optional<AlgorithmResult> bruteForceResult;
        std::optional<AlgorithmResult> greedyResult;
        std::optional<AlgorithmResult> hillClimbingResult;
    };

    Planifier(const Graph& graph, int numRobots, std::queue<Task> tasks);
    ~Planifier();

    // Getters
    int getNumRobots() const;
    int getAvailableRobots() const;
    int getBusyRobots() const;
    int getChargingRobots() const;
    const Graph& getGraph() const;
    std::queue<Task> getPendingTasks() const;

    // Setters
    void setNumRobots(int numRobots);
    void setAvailableRobots(int num);
    void setBusyRobots(int num);
    void setChargingRobots(int num);
    void setPendingTasks(int num);

    // Strategy Pattern: Set the planning algorithm
    void setAlgorithm(std::unique_ptr<Algorithm> algorithm);

    // Planning methods
    void plan(int algorithmChoice = 0);  // 0 = interactive, 1-4 = direct selection
    AlgorithmResult executePlan();       // Execute the currently set algorithm and return result


private:
    const Graph& G;
    std::queue<Robot> availableRobots;
    std::queue<Robot> busyRobots;
    std::queue<Robot> chargingRobots;
    const int totalRobots;
    std::queue<Task> pendingTasks;
    
    // Strategy Pattern: Current planning algorithm
    std::unique_ptr<Algorithm> currentAlgorithm;
};

#endif // PLANIFIER_H