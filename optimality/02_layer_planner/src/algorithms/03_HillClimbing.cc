#include "../../include/algorithms/03_HillClimbing.hh"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <random>
#include <algorithm>
using namespace std;

string HillClimbing::getName() const {
    return "Hill Climbing";
}

string HillClimbing::getDescription() const {
    return "Improves greedy solution through local search: inter-robot swaps/moves + intra-robot task ordering";
}

/**
 * @brief Main execution method for Hill Climbing algorithm
 */
void HillClimbing::execute(
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
    
    if (!compactMode) cout << "Executing Hill Climbing Algorithm..." << endl;
    
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
    vector<Robot> robots;
    while (!availableRobots.empty()) {
        Robot robot = availableRobots.front();
        availableRobots.pop();
        robots.push_back(robot);
        
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
             << robotStates.size() << " robots using hill climbing strategy." << endl;
    }

    // Get configuration
    if (robotStates.empty()) return;
    BatteryConfig config(robotStates[0].robot);
    
    // Get charging node
    int chargingNodeId = getChargingNodeId(graph);
    if (chargingNodeId == -1 && !compactMode) {
        cerr << "Warning: No charging node found in graph. Battery constraints ignored." << endl;
    }

    // Phase 1: Generate initial greedy solution
    if (!compactMode) cout << "Phase 1: Generating initial greedy solution..." << endl;
    vector<vector<Task>> assignment = generateGreedySolution(
        robotStates, tasksVec, graph, config, chargingNodeId
    );
    
    double currentMakespan = calculateMakespan(assignment, robots, graph, config, chargingNodeId);
    if (!compactMode) cout << "Initial greedy makespan: " << fixed << setprecision(2) << currentMakespan << "s" << endl;

    // Phase 2: Hill climbing improvement
    if (!compactMode) cout << "Phase 2: Improving solution through local search..." << endl;
    int improvementCount = 0;
    int maxIterations = 100;
    int iterationsWithoutImprovement = 0;
    int maxWithoutImprovement = 20;
    
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        bool improved = tryImprovement(assignment, robots, graph, config, chargingNodeId, currentMakespan);
        
        if (improved) {
            improvementCount++;
            iterationsWithoutImprovement = 0;
            if (!compactMode) {
                cout << "  Iteration " << iteration + 1 << ": Improved makespan to " 
                     << fixed << setprecision(2) << currentMakespan << "s" << endl;
            }
        } else {
            iterationsWithoutImprovement++;
            if (iterationsWithoutImprovement >= maxWithoutImprovement) {
                if (!compactMode) cout << "No improvement for " << maxWithoutImprovement << " iterations. Stopping." << endl;
                break;
            }
        }
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration = endTime - startTime;

    if (!compactMode) cout << "\n--- Hill Climbing Result ---" << endl;
    cout << "Algorithm computation time: " << duration.count() << " ms" << endl;
    cout << "Total improvements: " << improvementCount << endl;
    cout << "Final makespan (max robot completion time): " << fixed << setprecision(2) 
         << currentMakespan << " seconds." << endl;

    // Print beautified assignment
    if (!compactMode) printBeautifiedAssignment(robots, assignment, graph);

    // Update robot states
    for (size_t i = 0; i < robots.size(); ++i) {
        if (!assignment[i].empty()) {
            robots[i].setCurrentTask(assignment[i][0].getTaskId());
            busyRobots.push(robots[i]);
        } else {
            availableRobots.push(robots[i]);
        }
    }
    
    if (!compactMode) cout << "--------------------------" << endl;
}

/**
 * @brief Generate initial greedy solution
 */
vector<vector<Task>> HillClimbing::generateGreedySolution(
    vector<RobotState>& robotStates,
    const vector<Task>& tasks,
    const Graph& graph,
    const BatteryConfig& config,
    int chargingNodeId
) {
    vector<vector<Task>> assignment(robotStates.size());
    
    // Greedy: assign each task to robot that can complete it soonest
    for (const Task& task : tasks) {
        int bestRobotIdx = -1;
        double minCompletionTime = numeric_limits<double>::max();

        for (size_t i = 0; i < robotStates.size(); ++i) {
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
            
            if (!originNode || !destNode) continue;

            // Calculate completion time for this robot
            TaskBatteryInfo taskInfo = calculateTaskBatteryConsumption(
                robotStates[i].currentPosition, originNode, destNode,
                robotStates[i].currentBattery, config
            );

            double completionTime = robotStates[i].totalTime;
            
            // Check if charging needed
            if (chargingNodeId != -1 && shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                performCharging(robotStates[i].currentPosition, robotStates[i].currentBattery,
                               completionTime, chargingNodeId, graph, config);
                taskInfo = calculateTaskBatteryConsumption(
                    robotStates[i].currentPosition, originNode, destNode,
                    robotStates[i].currentBattery, config
                );
            }

            completionTime += taskInfo.timeToOrigin + taskInfo.timeForTask;

            if (completionTime < minCompletionTime) {
                minCompletionTime = completionTime;
                bestRobotIdx = static_cast<int>(i);
            }
        }

        if (bestRobotIdx != -1) {
            assignment[bestRobotIdx].push_back(task);
            
            // Update robot state
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
            if (destNode) {
                robotStates[bestRobotIdx].currentPosition = destNode->coordinates;
                robotStates[bestRobotIdx].assignedTasks.push_back(task);
            }
        }
    }

    return assignment;
}

/**
 * @brief Calculate makespan for an assignment
 */
double HillClimbing::calculateMakespan(
    const vector<vector<Task>>& assignment,
    const vector<Robot>& robots,
    const Graph& graph,
    const BatteryConfig& config,
    int chargingNodeId
) {
    double maxTime = 0.0;

    for (size_t i = 0; i < robots.size(); ++i) {
        if (assignment[i].empty()) continue;

        pair<double, double> currentPos = robots[i].getPosition();
        double currentBattery = robots[i].getBatteryLevel();
        double totalTime = 0.0;

        for (const Task& task : assignment[i]) {
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
            
            if (!originNode || !destNode) continue;

            TaskBatteryInfo taskInfo = calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            // Check if charging needed
            if (chargingNodeId != -1 && shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                performCharging(currentPos, currentBattery, totalTime,
                               chargingNodeId, graph, config);
                taskInfo = calculateTaskBatteryConsumption(
                    currentPos, originNode, destNode, currentBattery, config
                );
            }

            totalTime += taskInfo.timeToOrigin + taskInfo.timeForTask;
            currentBattery -= taskInfo.totalBatteryNeeded;
            currentPos = destNode->coordinates;
        }

        maxTime = max(maxTime, totalTime);
    }

    return maxTime;
}

/**
 * @brief Try to improve solution by swapping tasks between robots
 */
bool HillClimbing::tryImprovement(
    vector<vector<Task>>& assignment,
    const vector<Robot>& robots,
    const Graph& graph,
    const BatteryConfig& config,
    int chargingNodeId,
    double& currentMakespan
) {
    // Try swapping tasks between different robots
    for (size_t i = 0; i < assignment.size(); ++i) {
        for (size_t j = i + 1; j < assignment.size(); ++j) {
            if (assignment[i].empty() || assignment[j].empty()) continue;

            // Try swapping each task from robot i with each task from robot j
            for (size_t ti = 0; ti < assignment[i].size(); ++ti) {
                for (size_t tj = 0; tj < assignment[j].size(); ++tj) {
                    // Swap tasks
                    Task temp = assignment[i][ti];
                    assignment[i][ti] = assignment[j][tj];
                    assignment[j][tj] = temp;

                    // Calculate new makespan
                    double newMakespan = calculateMakespan(assignment, robots, graph, config, chargingNodeId);

                    if (newMakespan < currentMakespan) {
                        // Accept the swap
                        currentMakespan = newMakespan;
                        return true;
                    } else {
                        // Revert the swap
                        assignment[j][tj] = assignment[i][ti];
                        assignment[i][ti] = temp;
                    }
                }
            }
        }
    }

    // Try moving tasks between robots
    for (size_t i = 0; i < assignment.size(); ++i) {
        for (size_t j = 0; j < assignment.size(); ++j) {
            if (i == j || assignment[i].empty()) continue;

            for (size_t ti = 0; ti < assignment[i].size(); ++ti) {
                // Move task from robot i to robot j
                Task task = assignment[i][ti];
                assignment[i].erase(assignment[i].begin() + ti);
                assignment[j].push_back(task);

                double newMakespan = calculateMakespan(assignment, robots, graph, config, chargingNodeId);

                if (newMakespan < currentMakespan) {
                    currentMakespan = newMakespan;
                    return true;
                } else {
                    // Revert
                    assignment[j].pop_back();
                    assignment[i].insert(assignment[i].begin() + ti, task);
                }
            }
        }
    }

    // Try swapping tasks within the same robot (optimize execution order)
    for (size_t i = 0; i < assignment.size(); ++i) {
        if (assignment[i].size() < 2) continue;  // Need at least 2 tasks to swap

        // Try swapping each pair of tasks within this robot's queue
        for (size_t ti = 0; ti < assignment[i].size(); ++ti) {
            for (size_t tj = ti + 1; tj < assignment[i].size(); ++tj) {
                // Swap tasks within the same robot
                Task temp = assignment[i][ti];
                assignment[i][ti] = assignment[i][tj];
                assignment[i][tj] = temp;

                // Calculate new makespan
                double newMakespan = calculateMakespan(assignment, robots, graph, config, chargingNodeId);

                if (newMakespan < currentMakespan) {
                    // Accept the swap
                    currentMakespan = newMakespan;
                    return true;
                } else {
                    // Revert the swap
                    assignment[i][tj] = assignment[i][ti];
                    assignment[i][ti] = temp;
                }
            }
        }
    }

    return false;
}

// --- Helper Function Definitions (same as Greedy/BruteForce) ---

double HillClimbing::calculateDistance(pair<double, double> pos1, pair<double, double> pos2) const {
    return sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

int HillClimbing::getChargingNodeId(const Graph& graph) {
    for (int i = 0; i < graph.getNumVertices(); ++i) {
        const Graph::Node* node = graph.getNode(i);
        if (node && node->type == Graph::NodeType::Charging) {
            return node->nodeId;
        }
    }
    return -1;
}

HillClimbing::TaskBatteryInfo HillClimbing::calculateTaskBatteryConsumption(
    const pair<double, double>& currentPos,
    const Graph::Node* originNode,
    const Graph::Node* destNode,
    double currentBattery,
    const BatteryConfig& config
) const {
    TaskBatteryInfo info;
    
    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
    
    info.distanceToOrigin = calculateDistance(currentPos, originNode->coordinates);
    info.timeToOrigin = info.distanceToOrigin / config.robotSpeed;
    info.batteryToOrigin = info.timeToOrigin * percentageConsumePerSecond * config.alpha;
    
    info.distanceForTask = calculateDistance(originNode->coordinates, destNode->coordinates);
    info.timeForTask = info.distanceForTask / config.robotSpeed;
    info.batteryForTask = info.timeForTask * percentageConsumePerSecond;
    
    info.totalBatteryNeeded = info.batteryToOrigin + info.batteryForTask;
    info.batteryAfterTask = currentBattery - info.totalBatteryNeeded;
    
    return info;
}

bool HillClimbing::shouldCharge(double batteryAfterTask, double threshold) const {
    return batteryAfterTask < threshold;
}

void HillClimbing::performCharging(
    pair<double, double>& currentPos,
    double& currentBattery,
    double& totalTime,
    int chargingNodeId,
    const Graph& graph,
    const BatteryConfig& config
) {
    const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
    if (!chargingNode) return;
    
    double percentageConsumePerSecond = 100.0 / config.batteryLifeSpan;
    
    double distToCharging = calculateDistance(currentPos, chargingNode->coordinates);
    double timeToCharging = distToCharging / config.robotSpeed;
    
    currentBattery -= timeToCharging * percentageConsumePerSecond * config.alpha;
    totalTime += timeToCharging;
    
    double batteryNeeded = config.fullBattery - currentBattery;
    double chargingTime = batteryNeeded / config.batteryRechargeRate;
    currentBattery = config.fullBattery;
    totalTime += chargingTime;
    
    currentPos = chargingNode->coordinates;
}

void HillClimbing::printBeautifiedAssignment(
    vector<Robot>& robots,
    const vector<vector<Task>>& assignment,
    const Graph& graph
) {
    if (robots.empty()) return;
    
    BatteryConfig config(robots[0]);
    int chargingNodeId = getChargingNodeId(graph);

    cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
    cout << "║           HILL CLIMBING ASSIGNMENT REPORT                      ║" << endl;
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

            TaskBatteryInfo taskInfo = calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            if (chargingNodeId != -1 && shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
                if (chargingNode) {
                    cout << "│" << endl;
                    cout << "│  ⚡ CHARGING DECISION (Preventive)" << endl;
                    cout << "│    Current battery: " << fixed << setprecision(2) << currentBattery << "%" << endl;
                    cout << "│    Next task (ID " << task.getTaskId() << ") would consume: " 
                         << fixed << setprecision(2) << taskInfo.totalBatteryNeeded << "%" << endl;
                    cout << "│    Battery after task would be: " << fixed << setprecision(2) 
                         << taskInfo.batteryAfterTask << "% (BELOW " << config.lowBatteryThreshold << "% threshold)" << endl;
                    cout << "│    Decision: Go to charging station before attempting task" << endl;
                    cout << "│" << endl;
                    
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
                    
                    performCharging(currentPos, currentBattery, cumulativeTime, chargingNodeId, graph, config);
                    
                    taskInfo = calculateTaskBatteryConsumption(
                        currentPos, originNode, destNode, currentBattery, config
                    );
                }
            }

            currentBattery -= taskInfo.totalBatteryNeeded;
            cumulativeTime += taskInfo.timeToOrigin + taskInfo.timeForTask;

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

            currentPos = destNode->coordinates;
        }

        cout << "│" << endl;
        cout << "│  ★ Total completion time: " << fixed << setprecision(2) << cumulativeTime << "s" << endl;
        cout << "│  ★ Final battery level:   " << fixed << setprecision(2) << currentBattery << "%" << endl;
        cout << "└────────────────────────────────────────────────────────\n" << endl;
    }
}
