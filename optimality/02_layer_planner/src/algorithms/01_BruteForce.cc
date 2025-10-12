#include "../../include/algorithms/01_BruteForce.hh"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
using namespace std;

string BruteForce::getName() const {
    return "Brute Force";
}

string BruteForce::getDescription() const {
    return "Truly optimal brute force: Finds optimal partition AND optimal task ordering (solves TSP per robot)";
}


// --- Main Method Implementation ---


// Let N be the number of Robots and M the number of Tasks
// The cost is O(N^M * (M/N)!)
// 
// This algorithm finds the optimal PARTITION and optimal SCHEDULE:
// - For each partition (N^M combinations), it solves the TSP for each robot
// - Each robot's task ordering is optimized by checking all permutations
// - This ensures we find the truly optimal makespan
// 
// COMPLEXITY WARNING: With M tasks split among N robots, each robot gets ~M/N tasks.
// For each partition, we must check (M/N)! permutations per robot.
// Example: 12 tasks, 2 robots = 2^12 partitions * 6! * 6! ≈ 2 billion operations
void BruteForce::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots,
    bool compactMode
) {
    (void)chargingRobots; // Unused in brute force
    (void)totalRobots;    // Unused in brute force
    
    if (!compactMode) cout << "Executing Brute Force Algorithm..." << endl;
    
    // --- 1. PREPARATION ---
    auto startTime = chrono::high_resolution_clock::now();

    if (pendingTasks.empty()) {
        if (!compactMode) cout << "No pending tasks to assign." << endl;
        return;
    }
    if (availableRobots.empty()) {
        if (!compactMode) cout << "No available robots to perform tasks." << endl;
        return;
    }
    
    // Convert queues to vectors for easier processing (indexed access)
    vector<Robot> robotsVec;
    while (!availableRobots.empty()) {
        robotsVec.push_back(availableRobots.front());
        availableRobots.pop();
    }
    
    vector<Task> tasksVec;
    while (!pendingTasks.empty()) {
        tasksVec.push_back(pendingTasks.front());
        pendingTasks.pop(); // All tasks are now considered for assignment
    }

    if (!compactMode) cout << "Assigning " << tasksVec.size() << " tasks to " << robotsVec.size() << " available robots." << endl;
    
    // Calculate complexity and check feasibility
    // For truly optimal brute force: 
    // - Partition complexity: numRobots^numTasks
    // - TSP complexity per robot: factorial of tasks per robot
    // - Total: R^T * (T/R)! per robot ≈ R^T * ((T/R)!)^R
    // 
    // Practical limits:
    // - 8 tasks, 2 robots: 2^8 * 4! * 4! = 256 * 24 * 24 ≈ 147k operations (fast)
    // - 10 tasks, 2 robots: 2^10 * 5! * 5! = 1024 * 120 * 120 ≈ 15M operations (slow but feasible)
    // - 12 tasks, 2 robots: 2^12 * 6! * 6! = 4096 * 720 * 720 ≈ 2.1B operations (VERY SLOW)
    
    double logPartitionComplexity = tasksVec.size() * log(robotsVec.size());
    double estimatedPartitions = exp(logPartitionComplexity);
    
    // Estimate average tasks per robot and factorial complexity
    int tasksPerRobot = (tasksVec.size() + robotsVec.size() - 1) / robotsVec.size(); // Ceiling division
    double factorialComplexity = 1.0;
    for (int i = 2; i <= tasksPerRobot; ++i) {
        factorialComplexity *= i;
    }
    // Approximate total for all robots (simplified)
    double tspComplexity = pow(factorialComplexity, robotsVec.size());
    double estimatedComplexity = estimatedPartitions * tspComplexity;
    
    const double MAX_COMPLEXITY = 5e7; // 50 million operations (allows up to 10 tasks with 2 robots)
    
    if (estimatedComplexity > MAX_COMPLEXITY) {
        cout << "\n❌ ERROR: Truly optimal brute force complexity too high for this problem!" << endl;
        cout << "   Problem size: " << robotsVec.size() << " robots, " << tasksVec.size() << " tasks" << endl;
        cout << "   Partition complexity: " << robotsVec.size() << "^" << tasksVec.size() 
             << " ≈ " << (long long)(estimatedPartitions) << " partitions" << endl;
        cout << "   TSP complexity per partition: ~" << tasksPerRobot << "! per robot" << endl;
        cout << "   Total estimated operations: ";
        cout << "   Total estimated operations: ";
        if (estimatedComplexity > 1e15) {
            cout << " (astronomical - would take days/weeks)" << endl;
        } else if (estimatedComplexity > 1e12) {
            cout << (long long)(estimatedComplexity / 1e12) << " trillion" << endl;
        } else if (estimatedComplexity > 1e9) {
            cout << (long long)(estimatedComplexity / 1e9) << " billion" << endl;
        } else {
            cout << (long long)(estimatedComplexity / 1e6) << " million" << endl;
        }
        cout << "   Maximum feasible: ~" << (long long)(MAX_COMPLEXITY / 1e6) << " million operations" << endl;
        cout << "\n   Please use a faster algorithm:" << endl;
        cout << "     - Algorithm 2: Greedy (fast heuristic)" << endl;
        cout << "     - Algorithm 3: Hill Climbing (improved greedy)" << endl;
        cout << "\n   Aborting truly optimal brute force execution." << endl;
        
        // Return all robots to available queue
        for (const auto& robot : robotsVec) {
            availableRobots.push(robot);
        }
        // Return all tasks to pending queue
        for (const auto& task : tasksVec) {
            pendingTasks.push(task);
        }
        return;
    }
    
    // Warning for moderately large problem instances
    if (estimatedComplexity > 1e6 && estimatedComplexity <= MAX_COMPLEXITY) {
        cout << "\n⚠️  WARNING: This will take significant time..." << endl;
        cout << "    Partition complexity: " << robotsVec.size() << "^" << tasksVec.size() 
             << " ≈ " << (long long)(estimatedPartitions) << " partitions" << endl;
        cout << "    TSP per partition: ~" << tasksPerRobot << "! ≈ " 
             << (long long)factorialComplexity << " permutations per robot" << endl;
        cout << "    Total operations: ~" << (long long)(estimatedComplexity / 1e6) << " million" << endl;
        cout << "    Continuing... (press Ctrl+C to abort)\n" << endl;
    }
    
    // --- 2. BRUTE-FORCE ALGORITHM ---
    
    double minMakespan = numeric_limits<double>::max();
    vector<vector<Task>> bestAssignment(robotsVec.size());
    vector<vector<Task>> currentAssignment(robotsVec.size());
    
    // Start the recursive search for the best assignment
    findBestAssignment(0, tasksVec, robotsVec, graph, currentAssignment, bestAssignment, minMakespan);
    
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> algorithmDuration = endTime - startTime;

    // --- 3. OUTPUT AND STATE UPDATE ---
    
    if (!compactMode) cout << "\n--- Brute Force Result ---" << endl;
    cout << "Algorithm computation time: " << algorithmDuration.count() << " ms" << endl;
    
    if (minMakespan == numeric_limits<double>::max()) {
        if (!compactMode) cout << "No valid assignment could be found." << endl;
        // Push robots back to available queue if no assignment was made
        for (const auto& robot : robotsVec) {
            availableRobots.push(robot);
        }
        return;
    }

    cout << "Optimal assignment found with a makespan (max robot completion time) of: " 
         << minMakespan << " seconds." << endl;

    // Print beautified assignment
    if (!compactMode) printBeautifiedAssignment(robotsVec, bestAssignment, graph);

    // Update robot states
    for (size_t i = 0; i < robotsVec.size(); ++i) {
        Robot& robot = robotsVec[i];
        if (!bestAssignment[i].empty()) {
            // Assign the first task and update robot state
            robot.setCurrentTask(bestAssignment[i][0].getTaskId());
            busyRobots.push(robot);
        } else {
            // This robot was not assigned any task, remains available
            availableRobots.push(robot);
        }
    }
    if (!compactMode) cout << "--------------------------" << endl;
}


// --- Helper Function Definitions ---

/**
 * @brief Calculates the Euclidean distance between two points.
 */
double BruteForce::calculateDistance(pair<double, double> pos1, pair<double, double> pos2) const {
    return sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}

/**
 * @brief Gets the charging node ID from the graph.
 * @return The ID of the charging node, or -1 if not found.
 */
int BruteForce::getChargingNodeId(const Graph& graph) {
    for (int i = 0; i < graph.getNumVertices(); ++i) {
        const Graph::Node* node = graph.getNode(i);
        if (node && node->type == Graph::NodeType::Charging) {
            return node->nodeId;
        }
    }
    return -1; // No charging node found
}

/**
 * @brief Calculates battery consumption for a task.
 */
BruteForce::TaskBatteryInfo BruteForce::calculateTaskBatteryConsumption(
    const pair<double, double>& currentPos,
    const Graph::Node* originNode,
    const Graph::Node* destNode,
    double currentBattery,
    const BatteryConfig& config
) const {
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

/**
 * @brief Checks if battery level requires charging.
 */
bool BruteForce::shouldCharge(double batteryAfterTask, double threshold) const {
    return batteryAfterTask < threshold;
}

/**
 * @brief Performs charging operation and updates robot state.
 */
void BruteForce::performCharging(
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

/**
 * @brief Calculates the makespan (maximum completion time) for a complete assignment.
 * This version finds the OPTIMAL task ordering for each robot by checking all permutations.
 */
double BruteForce::calculateMakespan(
    const vector<vector<Task>>& assignment,
    vector<Robot>& robots,
    const Graph& graph
) {
    double maxTime = 0.0;

    for (size_t i = 0; i < robots.size(); ++i) {
        // For each robot, find the minimum possible time by checking all task orderings
        double robotMinTime = calculateMinTimeForRobot(robots[i], assignment[i], graph);
        
        if (robotMinTime > maxTime) {
            maxTime = robotMinTime;
        }
    }
    
    return maxTime;
}

/**
 * @brief Finds the minimum time for a single robot by checking all permutations of its tasks.
 * This solves the Traveling Salesman Problem (TSP) for the robot's assigned tasks.
 * 
 * @param robot The robot to calculate time for
 * @param tasks The tasks assigned to this robot
 * @param graph The warehouse graph
 * @return The minimum possible completion time for this robot
 */
double BruteForce::calculateMinTimeForRobot(
    const Robot& robot,
    const std::vector<Task>& tasks,
    const Graph& graph
) {
    if (tasks.empty()) {
        return 0.0;
    }

    // Create a mutable copy of tasks to generate permutations
    vector<Task> permutableTasks = tasks;

    // Sort the tasks to ensure std::next_permutation works correctly
    sort(permutableTasks.begin(), permutableTasks.end());

    double minTimeForThisRobot = numeric_limits<double>::max();
    BatteryConfig config(robot);
    int chargingNodeId = getChargingNodeId(graph);

    // Loop through every possible permutation of the tasks
    do {
        double timeForThisPermutation = 0.0;
        double currentBattery = robot.getBatteryLevel();
        pair<double, double> currentPos = robot.getPosition();
        bool validPermutation = true;

        // Execute tasks in this permutation order
        for (const auto& task : permutableTasks) {
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());
            
            if (!originNode || !destNode) {
                validPermutation = false;
                break;
            }

            TaskBatteryInfo taskInfo = calculateTaskBatteryConsumption(
                currentPos, originNode, destNode, currentBattery, config
            );

            // Check if charging is needed before this task
            if (chargingNodeId != -1 && shouldCharge(taskInfo.batteryAfterTask, config.lowBatteryThreshold)) {
                performCharging(currentPos, currentBattery, timeForThisPermutation, 
                              chargingNodeId, graph, config);
                
                // Recalculate task info from charging station
                taskInfo = calculateTaskBatteryConsumption(
                    currentPos, originNode, destNode, currentBattery, config
                );
            }
            
            // Check for battery failure (invalid permutation)
            if (taskInfo.batteryAfterTask < 0) {
                validPermutation = false;
                break;
            }

            // Execute the task
            timeForThisPermutation += taskInfo.timeToOrigin + taskInfo.timeForTask;
            currentBattery -= taskInfo.totalBatteryNeeded;
            currentPos = destNode->coordinates;
        }

        // After checking a full permutation, see if it's the best one yet
        if (validPermutation && timeForThisPermutation < minTimeForThisRobot) {
            minTimeForThisRobot = timeForThisPermutation;
        }

    } while (next_permutation(permutableTasks.begin(), permutableTasks.end()));

    return minTimeForThisRobot;
}


/**
 * @brief Recursively explores all possible task assignments to find the optimal one.
 * @param taskIndex The index of the current task in `tasks` to be assigned.
 */
void BruteForce::findBestAssignment(
    int taskIndex,
    const vector<Task>& tasks,
     vector<Robot>& robots,
    const Graph& graph,
    vector<vector<Task>>& currentAssignment,
    vector<vector<Task>>& bestAssignment,
    double& minMakespan
) {
    // BASE CASE: All tasks have been assigned.
    if (taskIndex == static_cast<int>(tasks.size())) {
        // Calculate the cost (makespan) of the current complete assignment.
        double currentMakespan = calculateMakespan(currentAssignment, robots, graph);
        
        // If this assignment is better than the best one found so far, update it.
        if (currentMakespan < minMakespan) {
            minMakespan = currentMakespan;
            bestAssignment = currentAssignment;
        }
        return;
    }

    // RECURSIVE STEP: Try assigning the current task to each robot.
    const Task& currentTask = tasks[taskIndex];
    for (size_t i = 0; i < robots.size(); ++i) {
        // Assign the task to robot 'i'.
        currentAssignment[i].push_back(currentTask);

        // Recurse to assign the next task.
        findBestAssignment(taskIndex + 1, tasks, robots, graph, currentAssignment, bestAssignment, minMakespan);

        // BACKTRACK: Remove the task from robot 'i' so it can be assigned
        // to the next robot in the next iteration of this loop.
        currentAssignment[i].pop_back();
    }
}


/**
 * @brief Outputs a beautified assignment report showing tasks for each robot.
 */
void BruteForce::printBeautifiedAssignment(
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
    cout << "║              BRUTE FORCE ASSIGNMENT REPORT                     ║" << endl;
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