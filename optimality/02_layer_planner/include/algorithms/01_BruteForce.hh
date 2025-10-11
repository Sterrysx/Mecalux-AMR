#ifndef BRUTEFORCE_HH
#define BRUTEFORCE_HH

#include "Algorithm.hh"
#include <string>

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
};

#endif // BRUTEFORCE_HH
