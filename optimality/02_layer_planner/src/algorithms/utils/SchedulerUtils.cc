#include "../../../include/algorithms/utils/SchedulerUtils.hh"
#include <cmath>

// --- BatteryConfig Constructor Implementation ---
SchedulerUtils::BatteryConfig::BatteryConfig(const Robot& robot, double speed) 
    : batteryLifeSpan(robot.getBatteryLifeSpan()),
      batteryRechargeRate(robot.getBatteryRechargeRate()),
      robotSpeed(speed),
      alpha(robot.getAlpha()),
      lowBatteryThreshold(20.0),
      fullBattery(100.0) {}

// --- Static Method Implementations ---

double SchedulerUtils::calculateDistance(std::pair<double, double> pos1, std::pair<double, double> pos2) {
    return sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

int SchedulerUtils::getChargingNodeId(const Graph& graph) {
    for (int i = 0; i < graph.getNumVertices(); ++i) {
        const Graph::Node* node = graph.getNode(i);
        if (node && node->type == Graph::NodeType::Charging) {
            return node->nodeId;
        }
    }
    return -1;
}

SchedulerUtils::TaskBatteryInfo SchedulerUtils::calculateTaskBatteryConsumption(
    const std::pair<double, double>& currentPos,
    const Graph::Node* originNode,
    const Graph::Node* destNode,
    double currentBattery,
    const BatteryConfig& config
) {
    TaskBatteryInfo info;
    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
    
    // Calculate travel to origin (empty robot, uses alpha)
    info.distanceToOrigin = calculateDistance(currentPos, originNode->coordinates);
    info.timeToOrigin = info.distanceToOrigin / config.robotSpeed;
    info.batteryToOrigin = info.timeToOrigin * percentageConsumePerSecond * config.alpha;
    
    // Calculate task execution (loaded robot, no alpha)
    info.distanceForTask = calculateDistance(originNode->coordinates, destNode->coordinates);
    info.timeForTask = info.distanceForTask / config.robotSpeed;
    info.batteryForTask = info.timeForTask * percentageConsumePerSecond;
    
    // Calculate totals
    info.totalBatteryNeeded = info.batteryToOrigin + info.batteryForTask;
    info.batteryAfterTask = currentBattery - info.totalBatteryNeeded;
    
    return info;
}

bool SchedulerUtils::shouldCharge(double batteryAfterTask, double threshold) {
    return batteryAfterTask < threshold;
}

void SchedulerUtils::performCharging(
    std::pair<double, double>& currentPos,
    double& currentBattery,
    double& totalTime,
    int chargingNodeId,
    const Graph& graph,
    const BatteryConfig& config
) {
    const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
    if (!chargingNode) return;
    
    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
    
    // 1. Travel to charging station
    double distToCharging = calculateDistance(currentPos, chargingNode->coordinates);
    double timeToCharging = distToCharging / config.robotSpeed;
    
    // Update battery for travel to charging station (without load, uses alpha)
    currentBattery -= timeToCharging * percentageConsumePerSecond * config.alpha;
    totalTime += timeToCharging;
    
    // 2. Charge battery to full
    double batteryNeeded = config.fullBattery - currentBattery;
    double chargingTime = batteryNeeded / config.batteryRechargeRate;
    currentBattery = config.fullBattery;
    totalTime += chargingTime;
    
    // 3. Update position to charging station
    currentPos = chargingNode->coordinates;
}
