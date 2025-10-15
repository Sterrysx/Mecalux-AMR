#include "../../../include/algorithms/utils/TSPSolver.hh"
#include "../../../include/algorithms/utils/SchedulerUtils.hh"
#include <algorithm>
#include <limits>

double TSPSolver::findOptimalSequenceTime(
    const Robot& robot,
    const std::vector<Task>& tasks,
    const Graph& graph
) {
    if (tasks.empty()) {
        return 0.0;
    }

    std::vector<Task> permutableTasks = tasks;
    std::sort(permutableTasks.begin(), permutableTasks.end());

    double minTimeForThisRobot = std::numeric_limits<double>::max();
    SchedulerUtils::BatteryConfig config(robot);
    int chargingNodeId = SchedulerUtils::getChargingNodeId(graph);

    do {
        double timeForThisPermutation = 0.0;
        double currentBattery = robot.getBatteryLevel();
        std::pair<double, double> currentPos = robot.getPosition();
        bool validPermutation = true;

        for (const auto& task : permutableTasks) {
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
            
            if (!originNode || !destNode) {
                validPermutation = false;
                break;
            }

            SchedulerUtils::TaskBatteryInfo taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                SchedulerUtils::performCharging(currentPos, currentBattery, timeForThisPermutation, 
                                                chargingNodeId, graph, config);
                
                taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                    currentPos, originNode, destNode, currentBattery, config
                );
            }
            
            if (taskInfo.batteryAfterTask < 0) {
                validPermutation = false;
                break;
            }

            timeForThisPermutation += taskInfo.timeToOrigin + taskInfo.timeForTask;
            currentBattery -= taskInfo.totalBatteryNeeded;
            currentPos = destNode->coordinates;
        }

        if (validPermutation && timeForThisPermutation < minTimeForThisRobot) {
            minTimeForThisRobot = timeForThisPermutation;
        }

    } while (std::next_permutation(permutableTasks.begin(), permutableTasks.end()));

    return minTimeForThisRobot;
}
