#ifndef HILLCLIMBING_HH
#define HILLCLIMBING_HH

#include "Algorithm.hh"
#include "utils/SchedulerUtils.hh"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief Hill Climbing algorithm implementation
 * 
 * This algorithm starts with a greedy initial solution and then iteratively
 * improves it by making local swaps between robot assignments. It explores
 * neighboring solutions and moves to better ones, escaping local optima through
 * random restarts if no improvement is found.
 * 
 * Strategy:
 * 1. Generate initial solution using greedy approach
 * 2. Try swapping tasks between robots
 * 3. Accept swap if it improves makespan
 * 4. Repeat until no improvement found
 * 5. Random restart if stuck
 */
class HillClimbing : public Algorithm {
public:
    HillClimbing() = default;
    ~HillClimbing() override = default;

    void execute(
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

private:
    // Robot state for simulation
    struct RobotState {
        Robot robot;
        std::pair<double, double> currentPosition;
        double currentBattery;
        double totalTime;
        std::vector<Task> assignedTasks;
    };

    // Generate initial greedy solution
    std::vector<std::vector<Task>> generateGreedySolution(
        std::vector<RobotState>& robotStates,
        const std::vector<Task>& tasks,
        const Graph& graph,
        const SchedulerUtils::BatteryConfig& config,
        int chargingNodeId
    );

    // Calculate makespan for a given assignment
    double calculateMakespan(
        const std::vector<std::vector<Task>>& assignment,
        const std::vector<Robot>& robots,
        const Graph& graph,
        const SchedulerUtils::BatteryConfig& config,
        int chargingNodeId
    );

    // Try to improve solution by swapping tasks
    bool tryImprovement(
        std::vector<std::vector<Task>>& assignment,
        const std::vector<Robot>& robots,
        const Graph& graph,
        const SchedulerUtils::BatteryConfig& config,
        int chargingNodeId,
        double& currentMakespan
    );
};

#endif // HILLCLIMBING_HH
