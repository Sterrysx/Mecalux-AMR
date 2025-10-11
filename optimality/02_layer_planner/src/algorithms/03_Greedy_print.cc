// This file contains the printBeautifiedAssignment function for Greedy algorithm
// Append this to 03_Greedy.cc

/**
 * @brief Outputs a beautified assignment report showing tasks for each robot.
 */
void Greedy::printBeautifiedAssignment(
    vector<Robot>& robots,
    const vector<vector<Task>>& assignment,
    const Graph& graph
) {
    if (robots.empty()) return;
    
    // Get configuration from first robot
    BatteryConfig config(robots[0]);
    
    // Get charging node
    int chargingNodeId = getChargingNodeId(graph);

    cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
    cout << "║              GREEDY ASSIGNMENT REPORT                          ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════════╝\n" << endl;

    for (size_t i = 0; i < robots.size(); ++i) {
        const Robot& robot = robots[i];
        cout << "┌─ Robot " << robot.getId() << " ─────────────────────────────────────────" << endl;
        cout << "│  Initial battery: " << fixed << setprecision(2) << robot.getBatteryLevel() << "%" << endl;
        
        if (assignment[i].empty()) {
            cout << "│  No tasks assigned" << endl;
            cout << "└────────────────────────────────────────────────────────\n" << endl;
            continue;
        }

        pair<double, double> currentPos = robot.getPosition();
        double cumulativeTime = 0.0;
        double currentBattery = robot.getBatteryLevel();

        for (size_t taskIdx = 0; taskIdx < assignment[i].size(); ++taskIdx) {
            const Task& task = assignment[i][taskIdx];
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                cout << "│  Task ID " << task.getTaskId() << ": ERROR - Invalid node" << endl;
                continue;
            }

            // Calculate battery consumption for this task
            TaskBatteryInfo taskInfo = calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            // Check if robot needs charging before this task
            if (chargingNodeId != -1 && shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
                if (chargingNode) {
                    // Display charging decision
                    cout << "│" << endl;
                    cout << "│  ⚡ CHARGING DECISION (Preventive)" << endl;
                    cout << "│    Current battery: " << fixed << setprecision(2) << currentBattery << "%" << endl;
                    cout << "│    Next task (ID " << task.getTaskId() << ") would consume: " 
                         << fixed << setprecision(2) << taskInfo.totalBatteryNeeded << "%" << endl;
                    cout << "│    Battery after task would be: " << fixed << setprecision(2) 
                         << taskInfo.batteryAfterTask << "% (BELOW " << config.lowBatteryThreshold << "% threshold)" << endl;
                    cout << "│    Decision: Go to charging station before attempting task" << endl;
                    cout << "│" << endl;
                    
                    // Calculate charging details for display
                    double distToCharging = calculateDistance(currentPos, chargingNode->coordinates);
                    double timeToCharging = distToCharging / config.robotSpeed;
                    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
                    double batteryOnArrival = currentBattery - (timeToCharging * percentageConsumePerSecond * config.alpha);
                    double batteryNeeded = config.fullBattery - batteryOnArrival;
                    double chargingTime = batteryNeeded / config.batteryRechargeRate;
                    
                    cout << "│  ⚡ CHARGING EVENT" << endl;
                    cout << "│    Travel to charging station (Node " << chargingNodeId << "): " 
                         << fixed << setprecision(2) << timeToCharging << "s" << endl;
                    cout << "│    Battery on arrival: " << fixed << setprecision(2) << batteryOnArrival << "%" << endl;
                    cout << "│    Charging time: " << fixed << setprecision(2) << chargingTime << "s" << endl;
                    cout << "│    Battery after charging: " << fixed << setprecision(2) << config.fullBattery << "%" << endl;
                    cout << "│" << endl;
                    
                    // Perform the charging
                    performCharging(currentPos, currentBattery, cumulativeTime, chargingNodeId, graph, config);
                    
                    // Recalculate task info from charging station
                    taskInfo = calculateTaskBatteryConsumption(
                        currentPos, originNode, destNode, currentBattery, config
                    );
                }
            }

            // Execute the task
            currentBattery -= taskInfo.totalBatteryNeeded;
            cumulativeTime += taskInfo.timeToOrigin + taskInfo.timeForTask;

            // Display task information
            cout << "│  Task ID " << task.getTaskId() << ":" << endl;
            cout << "│    From: Node " << task.getOriginNode() 
                 << " (" << originNode->coordinates.first << ", " << originNode->coordinates.second << ")" << endl;
            cout << "│    To:   Node " << task.getDestinationNode() 
                 << " (" << destNode->coordinates.first << ", " << destNode->coordinates.second << ")" << endl;
            cout << "│    Travel to origin: " << fixed << setprecision(2) << taskInfo.timeToOrigin << "s" << endl;
            cout << "│    Task execution:   " << fixed << setprecision(2) << taskInfo.timeForTask << "s" << endl;
            cout << "│    Cumulative time:  " << fixed << setprecision(2) << cumulativeTime << "s" << endl;
            cout << "│    Battery level:    " << fixed << setprecision(2) << currentBattery << "%" << endl;
            
            if (taskIdx < assignment[i].size() - 1) {
                cout << "│    ↓" << endl;
            }

            // Update current position for next task
            currentPos = destNode->coordinates;
        }

        cout << "│" << endl;
        cout << "│  ★ Total completion time: " << fixed << setprecision(2) << cumulativeTime << "s" << endl;
        cout << "│  ★ Final battery level:   " << fixed << setprecision(2) << currentBattery << "%" << endl;
        cout << "└────────────────────────────────────────────────────────\n" << endl;
    }
}
