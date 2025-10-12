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
        int totalRobots,
        bool compactMode = false
    ) override;

    std::string getName() const override;
    std::string getDescription() const override;

    // Calculates the Euclidean distance between two coordinate pairs.
    double calculateDistance(std::pair<double, double> pos1, std::pair<double, double> pos2) const;

    // Calculates the makespan for a given assignment of tasks to robots.
    double calculateMakespan(
        const std::vector<std::vector<Task>>& assignment,
         std::vector<Robot>& robots,
        const Graph& graph
    );

    // Helper method to get the charging node from the graph
    int getChargingNodeId(const Graph& graph);

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

    // Helper methods to reduce code duplication
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

    // New method for truly optimal solution: finds minimum time for a robot
    // by checking all permutations of its assigned tasks (solves TSP)
    double calculateMinTimeForRobot(
        const Robot& robot,
        const std::vector<Task>& tasks,
        const Graph& graph
    );
};

#endif // BRUTEFORCE_HH
