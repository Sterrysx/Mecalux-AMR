#include "../../../include/algorithms/utils/AssignmentPrinter.hh"
#include "../../../include/algorithms/utils/SchedulerUtils.hh"
#include <iostream>
#include <iomanip>

void AssignmentPrinter::printBeautifiedAssignment(
    const std::string& algorithmName,
    const std::vector<Robot>& robots,
    const std::vector<std::vector<Task>>& assignment,
    const Graph& graph
) {
    if (robots.empty()) return;
    
    SchedulerUtils::BatteryConfig config(robots[0]);
    int chargingNodeId = SchedulerUtils::getChargingNodeId(graph);
    
    // Convert algorithm name to uppercase for the header
    std::string upperAlgoName = algorithmName;
    for(char &c : upperAlgoName) c = toupper(c);
    
    std::string title = " " + upperAlgoName + " ASSIGNMENT REPORT ";
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║" << std::setw((64 + title.length()) / 2) << std::right << title
              << std::setw(64 - (64 + title.length()) / 2 + 1) << std::left << "║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n" << std::endl;

    for (size_t i = 0; i < robots.size(); ++i) {
        const Robot& robot = robots[i];
        std::cout << "┌─ Robot " << robot.getId() << " ─────────────────────────────────────────" << std::endl;
        std::cout << "│  Initial battery: " << std::fixed << std::setprecision(2) << robot.getBatteryLevel() << "%" << std::endl;
        
        if (assignment[i].empty()) {
            std::cout << "│  No tasks assigned" << std::endl;
            std::cout << "└────────────────────────────────────────────────────────\n" << std::endl;
            continue;
        }

        std::pair<double, double> currentPos = robot.getPosition();
        double cumulativeTime = 0.0;
        double currentBattery = robot.getBatteryLevel();

        for (size_t taskIdx = 0; taskIdx < assignment[i].size(); ++taskIdx) {
            const Task& task = assignment[i][taskIdx];
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                std::cout << "│  Task ID " << task.getTaskId() << ": ERROR - Invalid node" << std::endl;
                continue;
            }

            // Calculate battery consumption for this task
            SchedulerUtils::TaskBatteryInfo taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            // Check if robot needs charging before this task
            if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
                if (chargingNode) {
                    // Display charging decision
                    std::cout << "│" << std::endl;
                    std::cout << "│  ⚡ CHARGING DECISION (Preventive)" << std::endl;
                    std::cout << "│    Current battery: " << std::fixed << std::setprecision(2) << currentBattery << "%" << std::endl;
                    std::cout << "│    Next task (ID " << task.getTaskId() << ") would consume: " 
                         << std::fixed << std::setprecision(2) << taskInfo.totalBatteryNeeded << "%" << std::endl;
                    std::cout << "│    Battery after task would be: " << std::fixed << std::setprecision(2) 
                         << taskInfo.batteryAfterTask << "% (BELOW " << config.lowBatteryThreshold << "% threshold)" << std::endl;
                    std::cout << "│    Decision: Go to charging station before attempting task" << std::endl;
                    std::cout << "│" << std::endl;
                    
                    // Calculate charging details for display
                    double distToCharging = SchedulerUtils::calculateDistance(currentPos, chargingNode->coordinates);
                    double timeToCharging = distToCharging / config.robotSpeed;
                    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
                    double batteryOnArrival = currentBattery - (timeToCharging * percentageConsumePerSecond * config.alpha);
                    double batteryNeeded = config.fullBattery - batteryOnArrival;
                    double chargingTime = batteryNeeded / config.batteryRechargeRate;
                    
                    std::cout << "│  ⚡ CHARGING EVENT" << std::endl;
                    std::cout << "│    Travel to charging station (Node " << chargingNodeId << "): " 
                         << std::fixed << std::setprecision(2) << timeToCharging << "s" << std::endl;
                    std::cout << "│    Battery on arrival: " << std::fixed << std::setprecision(2) << batteryOnArrival << "%" << std::endl;
                    std::cout << "│    Charging time: " << std::fixed << std::setprecision(2) << chargingTime << "s" << std::endl;
                    std::cout << "│    Battery after charging: " << std::fixed << std::setprecision(2) << config.fullBattery << "%" << std::endl;
                    std::cout << "│" << std::endl;
                    
                    // Perform the charging
                    SchedulerUtils::performCharging(currentPos, currentBattery, cumulativeTime, chargingNodeId, graph, config);
                    
                    // Recalculate task info from charging station
                    taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                        currentPos, originNode, destNode, currentBattery, config
                    );
                }
            }

            // Execute the task
            currentBattery -= taskInfo.totalBatteryNeeded;
            cumulativeTime += taskInfo.timeToOrigin + taskInfo.timeForTask;

            // Display task information
            std::cout << "│  Task ID " << task.getTaskId() << ":" << std::endl;
            std::cout << "│    From: Node " << task.getOriginNode() 
                 << " (" << originNode->coordinates.first << ", " << originNode->coordinates.second << ")" << std::endl;
            std::cout << "│    To:   Node " << task.getDestinationNode() 
                 << " (" << destNode->coordinates.first << ", " << destNode->coordinates.second << ")" << std::endl;
            std::cout << "│    Travel to origin: " << std::fixed << std::setprecision(2) << taskInfo.timeToOrigin << "s" << std::endl;
            std::cout << "│    Task execution:   " << std::fixed << std::setprecision(2) << taskInfo.timeForTask << "s" << std::endl;
            std::cout << "│    Cumulative time:  " << std::fixed << std::setprecision(2) << cumulativeTime << "s" << std::endl;
            std::cout << "│    Battery level:    " << std::fixed << std::setprecision(2) << currentBattery << "%" << std::endl;
            
            if (taskIdx < assignment[i].size() - 1) {
                std::cout << "│    ↓" << std::endl;
            }

            // Update current position for next task
            currentPos = destNode->coordinates;
        }

        std::cout << "│" << std::endl;
        std::cout << "│  ★ Total completion time: " << std::fixed << std::setprecision(2) << cumulativeTime << "s" << std::endl;
        std::cout << "│  ★ Final battery level:   " << std::fixed << std::setprecision(2) << currentBattery << "%" << std::endl;
        std::cout << "└────────────────────────────────────────────────────────\n" << std::endl;
    }
}
