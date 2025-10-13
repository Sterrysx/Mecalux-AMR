#include "../../include/algorithms/01_BruteForce.hh"
#include "../../include/algorithms/utils/SchedulerUtils.hh"
#include "../../include/algorithms/utils/TSPSolver.hh"
#include "../../include/algorithms/utils/AssignmentPrinter.hh"
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


bool complexity_warnings(const vector<Task>& tasksVec, const vector<Robot>& robotsVec, bool compactMode) {
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
        
        return true; // Abort execution
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
    
    return false; // Continue execution
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
AlgorithmResult BruteForce::execute(
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
    
    AlgorithmResult result;
    result.algorithmName = getName();
    result.isOptimal = true; // Brute force is always optimal
    
    if (!compactMode) cout << "Executing Brute Force Algorithm..." << endl;
    
    // --- 1. PREPARATION ---
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
    
    if (complexity_warnings(tasksVec, robotsVec, compactMode)) {
        // Complexity too high - abort and return all robots and tasks to queues
        for (const auto& robot : robotsVec) {
            availableRobots.push(robot);
        }
        for (const auto& task : tasksVec) {
            pendingTasks.push(task);
        }
        
        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> algorithmDuration = endTime - startTime;
        result.makespan = numeric_limits<double>::max();
        result.computationTimeMs = algorithmDuration.count();
        return result;
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
        result.makespan = numeric_limits<double>::max();
        result.computationTimeMs = algorithmDuration.count();
        return result;
    }

    cout << "Optimal assignment found with a makespan (max robot completion time) of: " 
         << minMakespan << " seconds." << endl;

    // Print beautified assignment
    if (!compactMode) AssignmentPrinter::printBeautifiedAssignment(getName(), robotsVec, bestAssignment, graph);

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
    
    // Populate result struct
    result.makespan = minMakespan;
    result.assignment = bestAssignment;
    result.computationTimeMs = algorithmDuration.count();
    
    return result;
}


// --- Helper Function Definitions ---

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
        // For each robot, find the minimum possible time by using TSP solver
        double robotMinTime = TSPSolver::findOptimalSequenceTime(robots[i], assignment[i], graph);
        
        if (robotMinTime > maxTime) {
            maxTime = robotMinTime;
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
    if (taskIndex == tasks.size()) {
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