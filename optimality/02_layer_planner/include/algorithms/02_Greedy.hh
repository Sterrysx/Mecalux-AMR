#ifndef GREEDY_HH
#define GREEDY_HH

#include "Algorithm.hh"
#include "utils/SchedulerUtils.hh"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief Greedy planning algorithm implementation
 * 
 * This algorithm assigns each task to the robot that can complete it soonest,
 * considering current position, battery level, and potential charging needs.
 * Greedy criterion: minimize completion time for each task independently.
 */
class Greedy : public Algorithm {
public:
    Greedy() = default;
    ~Greedy() override = default;

    AlgorithmResult execute(
        const Graph& graph,
        std::queue<Robot>& availableRobots,
        std::queue<Robot>& busyRobots,
        std::queue<Robot>& chargingRobots,
        std::queue<Task>& pendingTasks,
        int totalRobots,
        bool compactMode = false
    ) override;

    std::string getName() const override;
    std::string getDescription() const override;
};

#endif // GREEDY_HH
