#include "../../include/algorithms/02_DynamicProgramming.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <map>

using namespace std;

// The DpState struct remains the same
struct DpState {
    double makespan = numeric_limits<double>::max();
    vector<double> robotWorkloads;
    vector<pair<double, double>> robotFinalPositions;
    vector<vector<Task>> assignment;
};

// Helper function to calculate Euclidean distance (can be moved into the class)
double calculateDistance(pair<double, double> pos1, pair<double, double> pos2) {
    return sqrt(pow(pos2.first - pos1.first, 2) + pow(pos2.second - pos1.second, 2));
}



// Helper to print the final assignment in a user-friendly format
void printBeautifiedAssignmentDP(
    const vector<Robot>& robots,
    const DpState& finalState,
    const Graph& graph
) {
    const double ROBOT_SPEED = 1.6;

    cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
    cout << "║          DYNAMIC PROGRAMMING ASSIGNMENT REPORT               ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════════╝\n" << endl;

    for (size_t i = 0; i < robots.size(); ++i) {
        const Robot& robot = robots[i];
        cout << "┌─ Robot " << robot.getId() << " ─────────────────────────────────────────" << endl;
        
        if (finalState.assignment[i].empty()) {
            cout << "│  No tasks assigned" << endl;
            cout << "└────────────────────────────────────────────────────────\n" << endl;
            continue;
        }

        pair<double, double> currentPos = robot.getPosition();
        double cumulativeTime = 0.0;

        for (size_t taskIdx = 0; taskIdx < finalState.assignment[i].size(); ++taskIdx) {
            const Task& task = finalState.assignment[i][taskIdx];
            const Graph::Node* originNode = graph.getNode(task.getOriginNode());
            const Graph::Node* destNode = graph.getNode(task.getDestinationNode());

            if (!originNode || !destNode) {
                cout << "│  Task ID " << task.getTaskId() << ": ERROR - Invalid node" << endl;
                continue;
            }

            // Time to travel to task origin
            double distToOrigin = calculateDistance(currentPos, originNode->coordinates);
            double timeToOrigin = distToOrigin / ROBOT_SPEED;
            
            // Time to complete the task (origin to destination)
            double distTask = calculateDistance(originNode->coordinates, destNode->coordinates);
            double taskTime = distTask / ROBOT_SPEED;
            
            cumulativeTime += timeToOrigin + taskTime;

            cout << "│  Task ID " << task.getTaskId() << ":" << endl;
            cout << "│    From: Node " << task.getOriginNode() 
                 << " (" << originNode->coordinates.first << ", " << originNode->coordinates.second << ")" << endl;
            cout << "│    To:   Node " << task.getDestinationNode() 
                 << " (" << destNode->coordinates.first << ", " << destNode->coordinates.second << ")" << endl;
            cout << "│    Travel to origin: " << fixed << setprecision(2) << timeToOrigin << "s" << endl;
            cout << "│    Task execution:   " << fixed << setprecision(2) << taskTime << "s" << endl;
            cout << "│    Cumulative time:  " << fixed << setprecision(2) << cumulativeTime << "s" << endl;
            
            if (taskIdx < finalState.assignment[i].size() - 1) {
                cout << "│    ↓" << endl;
            }

            // Update current position for next task
            currentPos = destNode->coordinates;
        }

        cout << "│" << endl;
        cout << "│  ★ Total completion time: " << fixed << setprecision(2) << cumulativeTime << "s" << endl;
        cout << "└────────────────────────────────────────────────────────\n" << endl;
    }
}


string DynamicProgramming::getName() const {
    return "Dynamic Programming";
}

string DynamicProgramming::getDescription() const {
    return "Uses bitmask DP to find the optimal assignment minimizing makespan";
}




// --- Main Method Implementation ---

void DynamicProgramming::execute(
    const Graph& graph,
    queue<Robot>& availableRobots,
    queue<Robot>& busyRobots,
    queue<Robot>& chargingRobots [[maybe_unused]],
    queue<Task>& pendingTasks,
    int totalRobots [[maybe_unused]]
) {
    cout << "Executing Optimal Dynamic Programming Algorithm..." << endl;
    auto startTime = chrono::high_resolution_clock::now();

    if (pendingTasks.empty() || availableRobots.empty()) {
        cout << "No pending tasks or available robots." << endl;
        return;
    }

    // --- 1. PREPARATION ---
    vector<Robot> robotsVec;
    while (!availableRobots.empty()) {
        robotsVec.push_back(availableRobots.front());
        availableRobots.pop();
    }
    vector<Task> tasksVec;
    while (!pendingTasks.empty()) {
        tasksVec.push_back(pendingTasks.front());
        pendingTasks.pop();
    }

    const int numTasks = tasksVec.size();
    const int numRobots = robotsVec.size();
    const double ROBOT_SPEED = 1.6;

    cout << "Assigning " << numTasks << " tasks to " << numRobots << " available robots." << endl;

    const int MAX_TASKS_DP_OPTIMAL = 14; // This is much slower, reduce limit
    if (numTasks > MAX_TASKS_DP_OPTIMAL) {
        cout << "\n❌ ERROR: Optimal DP is too slow for " << numTasks << " tasks." << endl;
        cout << "   Maximum feasible is around " << MAX_TASKS_DP_OPTIMAL << " tasks." << endl;
        for(const auto& r : robotsVec) availableRobots.push(r);
        for(const auto& t : tasksVec) pendingTasks.push(t);
        return;
    }
    
    // --- 2. OPTIMAL DYNAMIC PROGRAMMING ALGORITHM ---
    
    const int numStates = 1 << numTasks;
    // Main data structure: A vector of states for each mask
    vector<vector<DpState>> dp(numStates);

    // Initialize base case: mask = 0
    DpState initial_state;
    initial_state.makespan = 0.0;
    initial_state.robotWorkloads.assign(numRobots, 0.0);
    initial_state.assignment.resize(numRobots);
    for (const auto& robot : robotsVec) {
        initial_state.robotFinalPositions.push_back(robot.getPosition());
    }
    dp[0].push_back(initial_state);

    // Iterate through all masks
    for (int mask = 0; mask < numStates -1; ++mask) {
        if (dp[mask].empty()) continue; // Skip unreachable states

        // Find the next task to add (the first unset bit)
        int task_to_add_idx = 0;
        for(int i = 0; i < numTasks; ++i) {
            if (!((mask >> i) & 1)) {
                task_to_add_idx = i;
                break;
            }
        }
        
        int next_mask = mask | (1 << task_to_add_idx);
        const Task& currentTask = tasksVec[task_to_add_idx];
        const Graph::Node* originNode = graph.getNode(currentTask.getOriginNode());
        const Graph::Node* destNode = graph.getNode(currentTask.getDestinationNode());
        if (!originNode || !destNode) continue;
        
        vector<DpState> candidate_states;

        // For every non-dominated state in the current mask...
        for (const auto& prev_state : dp[mask]) {
            // ...try assigning the new task to every robot
            for (int k = 0; k < numRobots; ++k) {
                DpState new_state = prev_state;

                double dist_to_origin = calculateDistance(prev_state.robotFinalPositions[k], originNode->coordinates);
                double dist_task = calculateDistance(originNode->coordinates, destNode->coordinates);
                double time_for_task = (dist_to_origin + dist_task) / ROBOT_SPEED;

                new_state.robotWorkloads[k] += time_for_task;
                new_state.robotFinalPositions[k] = destNode->coordinates;
                new_state.assignment[k].push_back(currentTask);
                new_state.makespan = *max_element(new_state.robotWorkloads.begin(), new_state.robotWorkloads.end());

                candidate_states.push_back(new_state);
            }
        }
        
        // Pruning step: Keep only non-dominated states for the next_mask
        sort(candidate_states.begin(), candidate_states.end(), [](const DpState& a, const DpState& b){
            return a.makespan < b.makespan;
        });

        for(const auto& cand : candidate_states) {
            bool is_dominated = false;
            for(const auto& existing : dp[next_mask]) {
                bool all_workloads_worse = true;
                for(int i = 0; i < numRobots; ++i) {
                    if (cand.robotWorkloads[i] < existing.robotWorkloads[i] - 1e-9) { // Use tolerance for float comparison
                        all_workloads_worse = false;
                        break;
                    }
                }
                if (cand.makespan > existing.makespan - 1e-9 && all_workloads_worse) {
                    is_dominated = true;
                    break;
                }
            }
            if(!is_dominated) {
                dp[next_mask].push_back(cand);
            }
        }
    }


    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> algorithmDuration = endTime - startTime;

    // --- 3. OUTPUT AND STATE UPDATE ---
    const vector<DpState>& final_states = dp[numStates - 1];
    const DpState& bestState = *min_element(final_states.begin(), final_states.end(), 
        [](const DpState& a, const DpState& b) {
            return a.makespan < b.makespan;
    });

    cout << "\n--- Optimal Dynamic Programming Result ---" << endl;
    cout << "Algorithm computation time: " << algorithmDuration.count() << " ms" << endl;
    cout << "Optimal assignment found with a makespan of: " << bestState.makespan << " seconds." << endl;

    printBeautifiedAssignmentDP(robotsVec, bestState, graph);

    // Update robot queues
    for (int i = 0; i < numRobots; ++i) {
        Robot robot = robotsVec[i];
        if (!bestState.assignment[i].empty()) {
            robot.setCurrentTask(bestState.assignment[i][0].getTaskId());
            robot.setAvailability(false);
            busyRobots.push(robot);
        } else {
            availableRobots.push(robot);
        }
    }
    cout << "------------------------------------------" << endl;
}

