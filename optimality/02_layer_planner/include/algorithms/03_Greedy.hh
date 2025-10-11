#ifndef GREEDY_HH
#define GREEDY_HH

#include "Algorithm.hh"
#include <string>

/**
 * @brief Greedy planning algorithm implementation
 * 
 * This algorithm assigns tasks to the nearest available robot
 * without looking ahead (greedy heuristic).
 */
class Greedy : public Algorithm {
public:
    Greedy() = default;
    ~Greedy() override = default;

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

#endif // GREEDY_HH
