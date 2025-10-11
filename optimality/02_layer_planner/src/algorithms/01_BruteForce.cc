#include "../../include/algorithms/01_BruteForce.hh"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
using namespace std;

string BruteForce::getName() const {
    return "Brute Force";
}

string BruteForce::getDescription() const {
    return "Explores all possible task assignments to find the optimal solution";
}


// --- Main Method Implementation ---


// Let N be the number of Robots and M the number of Tasks
// The cost is O(N^M)
void BruteForce::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots,
    queue<Task>& pendingTasks,
    int totalRobots
) {
    (void)chargingRobots; // Unused in brute force
    (void)totalRobots;    // Unused in brute force
    
    cout << "Executing Brute Force Algorithm..." << endl;
    
    // --- 1. PREPARATION ---
    auto startTime = chrono::high_resolution_clock::now();

    if (pendingTasks.empty()) {
        cout << "No pending tasks to assign." << endl;
        return;
    }
    if (availableRobots.empty()) {
        cout << "No available robots to perform tasks." << endl;
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

    cout << "Assigning " << tasksVec.size() << " tasks to " << robotsVec.size() << " available robots." << endl;
    
    // Calculate complexity and check feasibility
    // For brute force: complexity = numRobots^numTasks
    // Safe limit: keep total combinations under ~100 million
    double logComplexity = tasksVec.size() * log(robotsVec.size());
    double estimatedComplexity = exp(logComplexity);
    const double MAX_COMPLEXITY = 1e8; // 100 million combinations
    
    if (estimatedComplexity > MAX_COMPLEXITY) {
        cout << "\n❌ ERROR: Brute force complexity too high for this problem!" << endl;
        cout << "   Problem size: " << robotsVec.size() << " robots, " << tasksVec.size() << " tasks" << endl;
        cout << "   Complexity: " << robotsVec.size() << "^" << tasksVec.size();
        if (estimatedComplexity > 1e15) {
            cout << " (astronomical)" << endl;
        } else if (estimatedComplexity > 1e12) {
            cout << " ≈ " << (long long)(estimatedComplexity / 1e12) << " trillion combinations" << endl;
        } else if (estimatedComplexity > 1e9) {
            cout << " ≈ " << (long long)(estimatedComplexity / 1e9) << " billion combinations" << endl;
        } else {
            cout << " ≈ " << (long long)(estimatedComplexity / 1e6) << " million combinations" << endl;
        }
        cout << "   Maximum feasible: ~" << (long long)(MAX_COMPLEXITY / 1e6) << " million combinations" << endl;
        cout << "\n   Please use a different algorithm:" << endl;
        cout << "     - Algorithm 2: Dynamic Programming" << endl;
        cout << "     - Algorithm 3: Greedy" << endl;
        cout << "\n   Aborting brute force execution." << endl;
        
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
    if (estimatedComplexity > 1e6) {
        cout << "\n⚠️  WARNING: This may take some time..." << endl;
        cout << "    Complexity: " << robotsVec.size() << "^" << tasksVec.size() 
             << " ≈ " << (long long)(estimatedComplexity / 1e6) << " million combinations" << endl;
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
    
    cout << "\n--- Brute Force Result ---" << endl;
    cout << "Algorithm computation time: " << algorithmDuration.count() << " ms" << endl;
    
    if (minMakespan == numeric_limits<double>::max()) {
        cout << "No valid assignment could be found." << endl;
        // Push robots back to available queue if no assignment was made
        for (const auto& robot : robotsVec) {
            availableRobots.push(robot);
        }
        return;
    }

    cout << "Optimal assignment found with a makespan (max robot completion time) of: " 
         << minMakespan << " seconds." << endl;

    // Print beautified assignment
    printBeautifiedAssignment(robotsVec, bestAssignment, graph);

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
    cout << "--------------------------" << endl;
}


// --- Helper Function Definitions ---

/**
 * @brief Calculates the Euclidean distance between two points.
 */
double BruteForce::calculateDistance(pair<double, double> pos1, pair<double, double> pos2) {
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
 * @brief Calculates the makespan (maximum completion time) for a complete assignment.
 * Includes battery management: robots must charge if battery drops below 20%.
 */
double BruteForce::calculateMakespan(
    const vector<vector<Task>>& assignment,
    vector<Robot>& robots,
    const Graph& graph
) {
    const double ROBOT_SPEED = 1.6; // m/s, as specified
    const double LOW_BATTERY_THRESHOLD = 20.0; // Battery threshold for charging
    const double BATTERY_RECHARGE_RATE = 0.5; // 0.5% per second (200 seconds for 0% to 100%)
    const double FULL_BATTERY = 100.0;
    double maxTime = 0.0;

    // Get the charging node
    int chargingNodeId = getChargingNodeId(graph);
    if (chargingNodeId == -1) {
        cerr << "Warning: No charging node found in graph. Battery constraints ignored." << endl;
    }

    for (size_t i = 0; i < robots.size(); ++i) {
        double robotTotalTime = 0.0;
        pair<double, double> currentPos = robots[i].getPosition();
        double currentBattery = robots[i].getBatteryLevel();

        for (const auto& task : assignment[i]) {
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                // Should not happen with a valid graph and tasks
                cerr << "Error: Invalid node ID in task " << task.getTaskId() << endl;
                continue; 
            }
            
            // Calculate battery consumption for this task
            const double BATTERY_LIFE_SPAN = robots[i].getBatteryLevel();
            const double ALPHA = robots[i].getAlpha();
            double percentageConsumePerSecond = 100.0 / BATTERY_LIFE_SPAN;
            
            double distToOrigin = calculateDistance(currentPos, originNode->coordinates);
            double timeToOrigin = distToOrigin / ROBOT_SPEED;
            double batteryToOrigin = timeToOrigin * percentageConsumePerSecond * ALPHA;
            
            double distTask = calculateDistance(originNode->coordinates, destNode->coordinates);
            double taskTime = distTask / ROBOT_SPEED;
            double batteryForTask = taskTime * percentageConsumePerSecond;
            
            double totalBatteryNeeded = batteryToOrigin + batteryForTask;
            double batteryAfterTask = currentBattery - totalBatteryNeeded;
            
            // Check if doing this task will bring battery below threshold
            if (chargingNodeId != -1 && batteryAfterTask < LOW_BATTERY_THRESHOLD) {
                const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
                if (chargingNode) {
                    // 1. Travel to charging station
                    double distToCharging = calculateDistance(currentPos, chargingNode->coordinates);
                    double timeToCharging = distToCharging / ROBOT_SPEED;
                    
                    // Update battery for travel to charging station (without load, so alpha = 2)
                    currentBattery -= timeToCharging * percentageConsumePerSecond * ALPHA;
                    robotTotalTime += timeToCharging;
                    
                    // 2. Charge battery to full
                    double batteryNeeded = FULL_BATTERY - currentBattery;
                    double chargingTime = batteryNeeded / BATTERY_RECHARGE_RATE;
                    currentBattery = FULL_BATTERY;
                    robotTotalTime += chargingTime;
                    
                    // Update position to charging station
                    currentPos = chargingNode->coordinates;
                    
                    // Recalculate battery consumption for task from charging station
                    distToOrigin = calculateDistance(currentPos, originNode->coordinates);
                    timeToOrigin = distToOrigin / ROBOT_SPEED;
                    batteryToOrigin = timeToOrigin * percentageConsumePerSecond * ALPHA;
                }
            }
            
            // 1. Time from robot's current position to task's origin
            currentBattery -= batteryToOrigin;
            robotTotalTime += timeToOrigin;
            
            // 2. Time from task's origin to its destination
            currentBattery -= batteryForTask;
            robotTotalTime += taskTime;

            // 3. Update robot's position for the next task
            currentPos = destNode->coordinates;
            
            // Safety check: if battery goes negative, this is an invalid assignment
            if (currentBattery < 0) {
                // This assignment is infeasible
                return numeric_limits<double>::max();
            }
        }

        if (robotTotalTime > maxTime) {
            maxTime = robotTotalTime;
        }
    }
    return maxTime;
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
    const double ROBOT_SPEED = 1.6; // m/s
    const double LOW_BATTERY_THRESHOLD = 20.0;
    const double BATTERY_RECHARGE_RATE = 0.5; // 0.5% per second
    const double FULL_BATTERY = 100.0;
    const double BATTERY_LIFE_SPAN = robots[0].getBatteryLevel();
    const double ALPHA = robots[0].getAlpha();

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
        double percentageConsumePerSecond = 100.0 / BATTERY_LIFE_SPAN;

        for (size_t taskIdx = 0; taskIdx < assignment[i].size(); ++taskIdx) {
            const Task& task = assignment[i][taskIdx];
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                cout << "│  Task ID " << task.getTaskId() << ": ERROR - Invalid node" << endl;
                continue;
            }

            // Calculate battery consumption for this task
            double distToOrigin = calculateDistance(currentPos, originNode->coordinates);
            double timeToOrigin = distToOrigin / ROBOT_SPEED;
            double batteryToOrigin = timeToOrigin * percentageConsumePerSecond * ALPHA;
            
            double distTask = calculateDistance(originNode->coordinates, destNode->coordinates);
            double taskTime = distTask / ROBOT_SPEED;
            double batteryForTask = taskTime * percentageConsumePerSecond;
            
            double totalBatteryNeeded = batteryToOrigin + batteryForTask;
            double batteryAfterTask = currentBattery - totalBatteryNeeded;

            // Check if robot needs charging before this task
            if (chargingNodeId != -1 && batteryAfterTask < LOW_BATTERY_THRESHOLD) {
                const Graph::Node* chargingNode = graph.getNode(chargingNodeId);
                if (chargingNode) {
                    cout << "│" << endl;
                    cout << "│  ⚡ CHARGING DECISION (Preventive)" << endl;
                    cout << "│    Current battery: " << fixed << setprecision(2) << currentBattery << "%" << endl;
                    cout << "│    Next task (ID " << task.getTaskId() << ") would consume: " 
                         << fixed << setprecision(2) << totalBatteryNeeded << "%" << endl;
                    cout << "│    Battery after task would be: " << fixed << setprecision(2) 
                         << batteryAfterTask << "% (BELOW " << LOW_BATTERY_THRESHOLD << "% threshold)" << endl;
                    cout << "│    Decision: Go to charging station before attempting task" << endl;
                    cout << "│" << endl;
                    
                    // Travel to charging station
                    double distToCharging = calculateDistance(currentPos, chargingNode->coordinates);
                    double timeToCharging = distToCharging / ROBOT_SPEED;
                    currentBattery -= timeToCharging * percentageConsumePerSecond * ALPHA;
                    cumulativeTime += timeToCharging;
                    
                    cout << "│  ⚡ CHARGING EVENT" << endl;
                    cout << "│    Travel to charging station (Node " << chargingNodeId << "): " 
                         << fixed << setprecision(2) << timeToCharging << "s" << endl;
                    cout << "│    Battery on arrival: " << fixed << setprecision(2) << currentBattery << "%" << endl;
                    
                    // Charge to full
                    double batteryNeeded = FULL_BATTERY - currentBattery;
                    double chargingTime = batteryNeeded / BATTERY_RECHARGE_RATE;
                    currentBattery = FULL_BATTERY;
                    cumulativeTime += chargingTime;
                    
                    cout << "│    Charging time: " << fixed << setprecision(2) << chargingTime << "s" << endl;
                    cout << "│    Battery after charging: " << fixed << setprecision(2) << currentBattery << "%" << endl;
                    
                    currentPos = chargingNode->coordinates;
                    cout << "│" << endl;
                    
                    // Recalculate from charging station
                    distToOrigin = calculateDistance(currentPos, originNode->coordinates);
                    timeToOrigin = distToOrigin / ROBOT_SPEED;
                    batteryToOrigin = timeToOrigin * percentageConsumePerSecond * ALPHA;
                }
            }

            // Time to travel to task origin
            currentBattery -= batteryToOrigin;
            
            // Time from task's origin to its destination
            currentBattery -= batteryForTask;
            
            cumulativeTime += timeToOrigin + taskTime;

            cout << "│  Task ID " << task.getTaskId() << ":" << endl;
            cout << "│    From: Node " << task.getOriginNode() 
                 << " (" << originNode->coordinates.first << ", " << originNode->coordinates.second << ")" << endl;
            cout << "│    To:   Node " << task.getDestinationNode() 
                 << " (" << destNode->coordinates.first << ", " << destNode->coordinates.second << ")" << endl;
            cout << "│    Travel to origin: " << fixed << setprecision(2) << timeToOrigin << "s" << endl;
            cout << "│    Task execution:   " << fixed << setprecision(2) << taskTime << "s" << endl;
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