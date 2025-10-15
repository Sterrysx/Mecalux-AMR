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
#include <unordered_map>
using namespace std;

string Greedy::getName() const {
    return "Greedy";
}

string Greedy::getDescription() const {
    return "Multi-start greedy: Tries 5 random task orderings, assigns each task to the robot that can complete it soonest";
}

// Helper function: Calculate makespan sequentially (same logic as HillClimbing::calculateMakespan)
namespace {
    double calculateMakespanSequential(
        const vector<vector<Task>>& assignment,
        const vector<Robot>& initialRobots,
        const Graph& graph,
        const SchedulerUtils::BatteryConfig& config,
        int chargingNodeId
    ) {
        double maxTime = 0.0;

        for (size_t i = 0; i < initialRobots.size(); ++i) {
            if (assignment[i].empty()) continue;

            // Simulate this robot's sequence from its starting state
            double totalTime = 0.0;
            double currentBattery = initialRobots[i].getBatteryLevel();
            pair<double, double> currentPos = initialRobots[i].getPosition();

            for (const Task& task : assignment[i]) {
                const Graph::Node* originNode = graph.getNode(task.getOriginNode());
                const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
                if (!originNode || !destNode) continue;

                auto taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                    currentPos, originNode, destNode, currentBattery, config
                );

                if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                    SchedulerUtils::performCharging(currentPos, currentBattery, totalTime, chargingNodeId, graph, config);
                    taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                        currentPos, originNode, destNode, currentBattery, config
                    );
                }

                totalTime += taskInfo.timeToOrigin + taskInfo.timeForTask;
                currentBattery -= taskInfo.totalBatteryNeeded;
                currentPos = destNode->coordinates;
            }

            if (totalTime > maxTime) {
                maxTime = totalTime;
            }
        }

        return maxTime;
    }
}

/**
 * @brief Main execution method for Greedy algorithm
 * 
 * Greedy Strategy: For each task, assign it to the robot that can complete it
 * the fastest, considering current position, battery, and charging needs.
 */
AlgorithmResult Greedy::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots,
    bool compactMode
) {
    (void)chargingRobots;
    (void)totalRobots;
    
    AlgorithmResult result;
    result.algorithmName = getName();
    result.isOptimal = false;
    
    if (!compactMode) cout << "Executing Greedy Algorithm..." << endl;
    
    auto startTime = chrono::high_resolution_clock::now();

    if (pendingTasks.empty()) {
        if (!compactMode) cout << "No pending tasks to assign." << endl;
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        return result;
    }
    if (availableRobots.empty()) {
        if (!compactMode) cout << "No available robots to perform tasks." << endl;
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        return result;
    }
    
    vector<Task> tasksVec;
    while (!pendingTasks.empty()) { 
        tasksVec.push_back(pendingTasks.front()); 
        pendingTasks.pop(); 
    }
    
    vector<Robot> originalRobots;
    while (!availableRobots.empty()) { 
        originalRobots.push_back(availableRobots.front()); 
        availableRobots.pop(); 
    }

    if (!compactMode) { 
        cout << "Assigning " << tasksVec.size() << " tasks to " 
             << originalRobots.size() << " robots using greedy strategy." << endl; 
    }

    if (originalRobots.empty()) {
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        return result;
    }
    
    SchedulerUtils::BatteryConfig config(originalRobots[0]);
    int chargingNodeId = SchedulerUtils::getChargingNodeId(graph);
    if (chargingNodeId == -1 && !compactMode) {
        cerr << "Warning: No charging node found in graph. Battery constraints ignored." << endl;
    }

    const int NUM_RANDOM_STARTS = 5;
    double bestMakespan = numeric_limits<double>::max();
    vector<vector<Task>> bestAssignment;
    
    for (int trial = 0; trial < NUM_RANDOM_STARTS; ++trial) {
        vector<Robot> trialRobots = originalRobots;
        vector<vector<Task>> trialAssignment(trialRobots.size());
        
        vector<Task> shuffledTasks = tasksVec;
        if (trial > 0) {
            unsigned seed = chrono::system_clock::now().time_since_epoch().count() + trial;
            shuffle(shuffledTasks.begin(), shuffledTasks.end(), default_random_engine(seed));
        }
        
        // This is the same logic as HillClimbing's generateGreedySolution
        unordered_map<int, double> robotCompletionTimes;
        for (const auto& task : shuffledTasks) {
            int bestRobotIdx = -1;
            double minEndTime = numeric_limits<double>::max();

            for (size_t i = 0; i < trialRobots.size(); ++i) {
                Robot tempRobot = trialRobots[i];
                double tempTime = robotCompletionTimes[tempRobot.getId()];
                
                const Graph::Node* originNode = graph.getNode(task.getOriginNode());
                const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
                if (!originNode || !destNode) continue;

                auto taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                    tempRobot.getPosition(), originNode, destNode, tempRobot.getBatteryLevel(), config);
                
                auto tempPos = tempRobot.getPosition();
                auto tempBat = tempRobot.getBatteryLevel();
                if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                    SchedulerUtils::performCharging(tempPos, tempBat, tempTime, chargingNodeId, graph, config);
                    taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(tempPos, originNode, destNode, tempBat, config);
                }
                double endTime = tempTime + taskInfo.timeToOrigin + taskInfo.timeForTask;

                if (endTime < minEndTime) {
                    minEndTime = endTime;
                    bestRobotIdx = i;
                }
            }

            if (bestRobotIdx != -1) {
                Robot& chosenRobot = trialRobots[bestRobotIdx];
                double& robotTime = robotCompletionTimes[chosenRobot.getId()];

                const Graph::Node* originNode = graph.getNode(task.getOriginNode());
                const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
                
                auto taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(
                    chosenRobot.getPosition(), originNode, destNode, chosenRobot.getBatteryLevel(), config);

                if (chargingNodeId != -1 && SchedulerUtils::shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                    auto robotPos = chosenRobot.getPosition();
                    auto robotBat = chosenRobot.getBatteryLevel();
                    SchedulerUtils::performCharging(robotPos, robotBat, robotTime, chargingNodeId, graph, config);
                    chosenRobot.setPosition(robotPos);
                    chosenRobot.setBatteryLevel(robotBat);
                    taskInfo = SchedulerUtils::calculateTaskBatteryConsumption(chosenRobot.getPosition(), originNode, destNode, chosenRobot.getBatteryLevel(), config);
                }
                
                robotTime += taskInfo.timeToOrigin + taskInfo.timeForTask;
                chosenRobot.setBatteryLevel(chosenRobot.getBatteryLevel() - taskInfo.totalBatteryNeeded);
                chosenRobot.setPosition(destNode->coordinates);
                trialAssignment[bestRobotIdx].push_back(task);
            }
        }
        
        // Use the correct sequential makespan calculator
        double trialMakespan = calculateMakespanSequential(trialAssignment, originalRobots, graph, config, chargingNodeId);

        if (trialMakespan < bestMakespan) {
            bestMakespan = trialMakespan;
            bestAssignment = trialAssignment;
        }
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> algorithmDuration = endTime - startTime;
    
    result.makespan = bestMakespan;
    result.assignment = bestAssignment;
    result.computationTimeMs = algorithmDuration.count();
    
    // Update robot queues - robots with tasks go to busyRobots, others to availableRobots
    for (size_t i = 0; i < originalRobots.size(); ++i) {
        if (!bestAssignment[i].empty()) {
            Robot robot = originalRobots[i];
            robot.setCurrentTask(bestAssignment[i][0].getTaskId());
            busyRobots.push(robot);
        } else {
            availableRobots.push(originalRobots[i]);
        }
    }
    
    if (!compactMode) {
        cout << "\n--- Greedy Algorithm Result ---" << endl;
        cout << "Algorithm computation time: " << result.computationTimeMs << " ms" << endl;
        cout << "Makespan (max robot completion time): " << fixed << setprecision(2) 
             << result.makespan << " seconds." << endl;
        AssignmentPrinter::printBeautifiedAssignment(getName(), originalRobots, bestAssignment, graph);
        cout << "--------------------------" << endl;
    }
    
    return result;
}
