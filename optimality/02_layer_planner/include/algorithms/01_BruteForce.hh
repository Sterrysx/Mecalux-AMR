#ifndef BRUTEFORCE_HH
#define BRUTEFORCE_HH

#include "Algorithm.hh"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief Brute Force planning algorithm implementation
 * 
 * This algorithm explores all possible task assignments to find
 * the optimal solution (placeholder for actual implementation).
 */
class BruteForce : public Algorithm {
public:
    BruteForce() = default;
    ~BruteForce() override = default;

    void execute(
        const Graph& graph,
        std::queue<Robot>& availableRobots,
        std::queue<Robot>& busyRobots,
        std::queue<Robot>& chargingRobots,
        std::queue<Task>& pendingTasks,
        int totalRobots
    ) override;

    std::string getName() const override;
    std::string getDescription() const override;

    // Calculates the Euclidean distance between two coordinate pairs.
    double calculateDistance(std::pair<double, double> pos1, std::pair<double, double> pos2);

    // Calculates the makespan for a given assignment of tasks to robots.
    double calculateMakespan(
        const std::vector<std::vector<Task>>& assignment,
        const std::vector<Robot>& robots,
        const Graph& graph
    );

    // Recursive function to find the best assignment using backtracking.
    void findBestAssignment(
        int taskIndex,
        const std::vector<Task>& tasks,
         std::vector<Robot>& robots,
        const Graph& graph,
        std::vector<std::vector<Task>>& currentAssignment,
        std::vector<std::vector<Task>>& bestAssignment,
        double& minMakespan
    );

    // Outputs a beautified assignment report
    void printBeautifiedAssignment(
        const std::vector<Robot>& robots,
        const std::vector<std::vector<Task>>& assignment,
        const Graph& graph
    );
};

#endif // BRUTEFORCE_HH
