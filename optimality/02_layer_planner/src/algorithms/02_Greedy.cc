#include "../../include/algorithms/02_Greedy.hh"
#include "../../include/algorithms/utils/SchedulerUtils.hh"
#include "../../include/algorithms/utils/AssignmentPrinter.hh"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <random>
#include <algorithm>
using namespace std;

string Greedy::getName() const {
    return "Greedy";
}

string Greedy::getDescription() const {
    return "Multi-start greedy: Tries 5 random task orderings, assigns each task to the robot that can complete it soonest";
}

/**
 * @brief Main execution method for Greedy algorithm
 * 
 * Greedy Strategy: For each task, assign it to the robot that can complete it
 * the fastest, considering current position, battery, and charging needs.
 */
void Greedy::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots,
    bool compactMode
) {
    (void)chargingRobots; // Unused
    (void)totalRobots;    // Unused
    
    if (!compactMode) cout << "Executing Greedy Algorithm..." << endl;
    
    auto startTime = chrono::high_resolution_clock::now();

    if (pendingTasks.empty()) {
        if (!compactMode) cout << "No pending tasks to assign." << endl;
        return;
    }
    if (availableRobots.empty()) {
        if (!compactMode) cout << "No available robots to perform tasks." << endl;
        return;
    }
    
    // Convert queues to vectors
    vector<Task> tasksVec;
    while (!pendingTasks.empty()) {
        tasksVec.push_back(pendingTasks.front());
        pendingTasks.pop();
    }

        // Initialize robot states
    vector<RobotState> robotStates;
    while (!availableRobots.empty()) {
        Robot robot = availableRobots.front();
        availableRobots.pop();
        
        RobotState state{
            Robot(robot.getId(), robot.getPosition(), robot.getBatteryLevel(), 
                  robot.getCurrentTask(), robot.getMaxSpeed(), robot.getLoadCapacity()),
            robot.getPosition(),
            robot.getBatteryLevel(),
            0.0,
            {}
        };
        robotStates.push_back(state);
    }

    if (!compactMode) {
        cout << "Assigning " << tasksVec.size() << " tasks to " 
             << robotStates.size() << " robots using greedy strategy." << endl;
    }

    // Get configuration from first robot
    if (robotStates.empty()) return;
    SchedulerUtils::BatteryConfig config(robotStates[0].robot);
    
    // Get charging node
    int chargingNodeId = SchedulerUtils::getChargingNodeId(graph);
    if (chargingNodeId == -1 && !compactMode) {
        cerr << "Warning: No charging node found in graph. Battery constraints ignored." << endl;
    }

    // Multi-start greedy: Try multiple random orderings to find best solution
    const int NUM_RANDOM_STARTS = 5;
    double bestMakespan = numeric_limits<double>::max();
    vector<vector<Task>> bestAssignment(robotStates.size());
    vector<pair<double, double>> bestPositions(robotStates.size());
    vector<double> bestBatteries(robotStates.size());
    vector<double> bestTimes(robotStates.size());
    
    for (int trial = 0; trial < NUM_RANDOM_STARTS; ++trial) {
        // Reset robot states for this trial
        vector<RobotState> trialRobotStates;
        for (size_t i = 0; i < robotStates.size(); ++i) {
            trialRobotStates.push_back(RobotState{
                Robot(robotStates[i].robot.getId(), robotStates[i].robot.getPosition(), 
                      robotStates[i].robot.getBatteryLevel(), robotStates[i].robot.getCurrentTask(),
                      robotStates[i].robot.getMaxSpeed(), robotStates[i].robot.getLoadCapacity()),
                robotStates[i].robot.getPosition(),
                robotStates[i].robot.getBatteryLevel(),
                0.0,
                {}
            });
        }
        
        // Shuffle tasks for random starts (except first trial which uses original order)
        vector<Task> shuffledTasks = tasksVec;
        if (trial > 0) {
            unsigned seed = chrono::system_clock::now().time_since_epoch().count() + trial;
            shuffle(shuffledTasks.begin(), shuffledTasks.end(), default_random_engine(seed));
        }
        
        // Greedy assignment: for each task, choose the robot that can complete it soonest
        for (const Task& task : shuffledTasks) {
            int bestRobotIdx = -1;
            double minCompletionTime = numeric_limits<double>::max();

            // Try each robot and find which one can complete this task soonest
            for (size_t i = 0; i < trialRobotStates.size(); ++i) {
                RobotState tempState = trialRobotStates[i]; // Make a copy for simulation
                
                double completionTime = calculateCompletionTime(
                    tempState, task, graph, config, chargingNodeId
                );

                if (completionTime < minCompletionTime) {
                    minCompletionTime = completionTime;
                    bestRobotIdx = static_cast<int>(i);
                }
            }

            // Assign task to the best robot
            if (bestRobotIdx != -1) {
                RobotState& robotState = trialRobotStates[bestRobotIdx];
            
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                cerr << "Error: Invalid node ID in task " << task.getTaskId() << endl;
                continue;
            }

            // Calculate battery consumption for this task
            SchedulerUtils::TaskBatteryInfo taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                robotState.currentPosition, originNode, destNode, 
                robotState.currentBattery, config
            );

            // Check if charging is needed
            if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                SchedulerUtils::performCharging(robotState.currentPosition, robotState.currentBattery,
                              robotState.totalTime, chargingNodeId, graph, config);
                
                // Recalculate after charging
                taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                    robotState.currentPosition, originNode, destNode,
                    robotState.currentBattery, config
                );
            }

            // Execute the task
            robotState.currentBattery -= taskInfo.totalBatteryNeeded;
            robotState.totalTime += taskInfo.timeToOrigin + taskInfo.timeForTask;
            robotState.currentPosition = destNode->coordinates;
            robotState.assignedTasks.push_back(task);
        }
    }
        
        // Calculate makespan for this trial
        double trialMakespan = 0.0;
        for (const auto& state : trialRobotStates) {
            if (state.totalTime > trialMakespan) {
                trialMakespan = state.totalTime;
            }
        }
        
        // Keep best solution
        if (trialMakespan < bestMakespan) {
            bestMakespan = trialMakespan;
            // Save best state data (not the Robot objects which can't be copied)
            for (size_t i = 0; i < trialRobotStates.size(); ++i) {
                bestAssignment[i] = trialRobotStates[i].assignedTasks;
                bestPositions[i] = trialRobotStates[i].currentPosition;
                bestBatteries[i] = trialRobotStates[i].currentBattery;
                bestTimes[i] = trialRobotStates[i].totalTime;
            }
        }
    }
    
    // Apply best solution found to robot states
    for (size_t i = 0; i < robotStates.size(); ++i) {
        robotStates[i].assignedTasks = bestAssignment[i];
        robotStates[i].currentPosition = bestPositions[i];
        robotStates[i].currentBattery = bestBatteries[i];
        robotStates[i].totalTime = bestTimes[i];
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> algorithmDuration = endTime - startTime;

    // Calculate makespan (maximum completion time)
    double makespan = bestMakespan;

    if (!compactMode) cout << "\n--- Greedy Algorithm Result ---" << endl;
    cout << "Algorithm computation time: " << algorithmDuration.count() << " ms" << endl;
    cout << "Makespan (max robot completion time): " << fixed << setprecision(2) 
         << makespan << " seconds." << endl;

    // Convert robot states to assignment format for printing
    vector<Robot> robots;
    vector<vector<Task>> assignment(robotStates.size());
    
    for (size_t i = 0; i < robotStates.size(); ++i) {
        robots.push_back(robotStates[i].robot);
        assignment[i] = robotStates[i].assignedTasks;
    }

    // Print beautified assignment
    if (!compactMode) AssignmentPrinter::printBeautifiedAssignment(getName(), robots, assignment, graph);

    // Update robot states
    for (size_t i = 0; i < robotStates.size(); ++i) {
        if (!assignment[i].empty()) {
            robotStates[i].robot.setCurrentTask(assignment[i][0].getTaskId());
            busyRobots.push(robotStates[i].robot);
        } else {
            availableRobots.push(robotStates[i].robot);
        }
    }
    
    if (!compactMode) cout << "--------------------------" << endl;
}


// --- Helper Function Definitions ---

/**
 * @brief Calculate how long it would take a robot to complete a task.
 * This is used for the greedy selection criterion.
 */
double Greedy::calculateCompletionTime(
    RobotState& robotState,
    const Task& task,
    const Graph& graph,
    const SchedulerUtils::BatteryConfig& config,
    int chargingNodeId
) {
    const Graph::Node* originNode = graph.getNode(task.getOriginNode());
    const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

    if (!originNode || !destNode) {
        return numeric_limits<double>::max(); // Invalid task
    }

    double simulatedTime = robotState.totalTime;
    double simulatedBattery = robotState.currentBattery;
    pair<double, double> simulatedPos = robotState.currentPosition;

    // Calculate battery consumption for this task
    SchedulerUtils::TaskBatteryInfo taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
        simulatedPos, originNode, destNode, simulatedBattery, config
    );

    // Check if charging is needed
    if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
        SchedulerUtils::performCharging(simulatedPos, simulatedBattery, simulatedTime,
                       chargingNodeId, graph, config);
        
        // Recalculate after charging
        taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
            simulatedPos, originNode, destNode, simulatedBattery, config
        );
    }

    // Add time to complete this task
    simulatedTime += taskInfo.timeToOrigin + taskInfo.timeForTask;

    return simulatedTime;
}
