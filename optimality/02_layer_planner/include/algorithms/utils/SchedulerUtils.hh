#ifndef SCHEDULER_UTILS_HH
#define SCHEDULER_UTILS_HH

#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"

// A utility class for common, stateless scheduling calculations.
class SchedulerUtils {
public:
    // --- NESTED STRUCTS ---
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

    struct BatteryConfig {
        double batteryLifeSpan;
        double batteryRechargeRate;
        double robotSpeed;
        double alpha;
        double lowBatteryThreshold;
        double fullBattery;
        
        // Constructor to initialize from a Robot object
        BatteryConfig(const Robot& robot, double speed = 1.6);
    };

    // --- STATIC HELPER METHODS ---
    static double calculateDistance(std::pair<double, double> pos1, std::pair<double, double> pos2);

    static int getChargingNodeId(const Graph& graph);

    static TaskBatteryInfo calculateTaskBatteryConsumption(
        const std::pair<double, double>& currentPos,
        const Graph::Node* originNode,
        const Graph::Node* destNode,
        double currentBattery,
        const BatteryConfig& config
    );

    static bool shouldCharge(double batteryAfterTask, double threshold);

    static void performCharging(
        std::pair<double, double>& currentPos,
        double& currentBattery,
        double& totalTime,
        int chargingNodeId,
        const Graph& graph,
        const BatteryConfig& config
    );
};

#endif // SCHEDULER_UTILS_HH
