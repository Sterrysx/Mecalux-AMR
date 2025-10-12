#ifndef GREEDY_HH
#define GREEDY_HH

#include "Algorithm.hh"
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

    // Configuration constants
    struct BatteryConfig {
        double lowBatteryThreshold;
        double fullBattery;
        double batteryLifeSpan;
        double batteryRechargeRate;
        double alpha;
        double robotSpeed;
        
        BatteryConfig(const Robot& robot, double speed = 1.6) 
            : lowBatteryThreshold(20.0),
              fullBattery(100.0),
              batteryLifeSpan(robot.getBatteryLifeSpan()),
              batteryRechargeRate(robot.getBatteryRechargeRate()),
              alpha(robot.getAlpha()),
              robotSpeed(speed) {}
    };

    // Robot state for greedy assignment
    struct RobotState {
        Robot robot;
        std::pair<double, double> currentPosition;
        double currentBattery;
        double totalTime;
        std::vector<Task> assignedTasks;
    };

    // Helper methods
    TaskBatteryInfo calculateTaskBatteryConsumption(
        const std::pair<double, double>& currentPos,
        const Graph::Node* originNode,
        const Graph::Node* destNode,
        double currentBattery,
        const BatteryConfig& config
    ) const;

    bool shouldCharge(double batteryAfterTask, double threshold) const;

    void performCharging(
        std::pair<double, double>& currentPos,
        double& currentBattery,
        double& totalTime,
        int chargingNodeId,
        const Graph& graph,
        const BatteryConfig& config
    );

    // Calculate completion time for a robot to complete a task
    double calculateCompletionTime(
        RobotState& robotState,
        const Task& task,
        const Graph& graph,
        const BatteryConfig& config,
        int chargingNodeId
    );
};

#endif // GREEDY_HH
