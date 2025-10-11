#ifndef DYNAMIC_PROGRAMMING_HH
#define DYNAMIC_PROGRAMMING_HH

#include "Algorithm.hh"
#include <string>

/**
 * @brief Dynamic Programming planning algorithm implementation
    *
 * This algorithm uses dynamic programming to find the optimal task
 * assignment by breaking the problem into smaller subproblems and
 * solving them recursively.
 */
class DynamicProgramming : public Algorithm {
public:
    DynamicProgramming() = default;
    ~DynamicProgramming() override = default;

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

#endif // DYNAMIC_PROGRAMMING_HH
