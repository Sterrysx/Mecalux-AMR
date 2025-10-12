#ifndef HILLCLIMBING_HH
#define HILLCLIMBING_HH

#include "Algorithm.hh"
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

    // Calculates the Euclidean distance between two coordinate pairs.
    double calculateDistance(std::pair<double, double> pos1, std::pair<double, double> pos2) const;

    // Helper method to get the charging node from the graph
    int getChargingNodeId(const Graph& graph);

    // Outputs a beautified assignment report
    void printBeautifiedAssignment(
        std::vector<Robot>& robots,
        const std::vector<std::vector<Task>>& assignment,
        const Graph& graph
    );

private:
    // Battery and task computation helper structure
    struct TaskBatteryInfo {
        double distanceToOrigin;
        double timeToOrigin;
        double batteryToOrigin;
        
        double distanceForTask;
        double timeForTask;
        double batteryForTask;
        
        double totalBatteryNeeded;
        double batteryAfterTask;
    };

    // Battery configuration structure
    struct BatteryConfig {
        double batteryLifeSpan;
        double batteryRechargeRate;
        double robotSpeed;
        double alpha;
        double lowBatteryThreshold;
        double fullBattery;
        
        BatteryConfig(const Robot& robot) 
            : batteryLifeSpan(robot.getBatteryLifeSpan()),
              batteryRechargeRate(robot.getBatteryRechargeRate()),
              robotSpeed(robot.getMaxSpeed()),
              alpha(2.0),
              lowBatteryThreshold(20.0),
              fullBattery(100.0) {}
    };

    // Robot state for simulation
    struct RobotState {
        Robot robot;
        std::pair<double, double> currentPosition;
        double currentBattery;
        double totalTime;
        std::vector<Task> assignedTasks;
    };

    // Calculate battery consumption for a task
    TaskBatteryInfo calculateTaskBatteryConsumption(
        const std::pair<double, double>& currentPos,
        const Graph::Node* originNode,
        const Graph::Node* destNode,
        double currentBattery,
        const BatteryConfig& config
    ) const;

    // Check if charging is needed
    bool shouldCharge(double batteryAfterTask, double threshold) const;

    // Perform charging operation
    void performCharging(
        std::pair<double, double>& currentPos,
        double& currentBattery,
        double& totalTime,
        int chargingNodeId,
        const Graph& graph,
        const BatteryConfig& config
    );

    // Generate initial greedy solution
    std::vector<std::vector<Task>> generateGreedySolution(
        std::vector<RobotState>& robotStates,
        const std::vector<Task>& tasks,
        const Graph& graph,
        const BatteryConfig& config,
        int chargingNodeId
    );

    // Calculate makespan for a given assignment
    double calculateMakespan(
        const std::vector<std::vector<Task>>& assignment,
        const std::vector<Robot>& robots,
        const Graph& graph,
        const BatteryConfig& config,
        int chargingNodeId
    );

    // Try to improve solution by swapping tasks
    bool tryImprovement(
        std::vector<std::vector<Task>>& assignment,
        const std::vector<Robot>& robots,
        const Graph& graph,
        const BatteryConfig& config,
        int chargingNodeId,
        double& currentMakespan
    );
};

#endif // HILLCLIMBING_HH
