/**
 * @file HillClimbing.cc
 * @brief Implementation of Hill Climbing VRP solver
 */

#include "../include/HillClimbing.hh"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <limits>
#include <algorithm>

namespace Backend {
namespace Layer2 {

// =============================================================================
// MAIN SOLVE METHOD
// =============================================================================

VRPResult HillClimbing::Solve(
    const std::vector<Task>& tasks,
    std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) {
    VRPResult result;
    result.algorithmName = GetName();
    result.isOptimal = false;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Handle edge cases
    if (tasks.empty()) {
        std::cout << "[HillClimbing] No tasks to assign.\n";
        result.isFeasible = true;
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        
        // Create empty itineraries for all robots
        for (const auto& robot : robots) {
            result.robotItineraries[robot.GetRobotId()] = {};
        }
        return result;
    }
    
    if (robots.empty()) {
        std::cout << "[HillClimbing] No robots available.\n";
        result.isFeasible = false;
        result.makespan = std::numeric_limits<double>::max();
        result.computationTimeMs = 0.0;
        return result;
    }
    
    std::cout << "[HillClimbing] Solving VRP: " << tasks.size() 
              << " tasks, " << robots.size() << " robots\n";
    
    // Phase 1: Generate initial greedy solution
    Assignment bestAssignment = GenerateGreedySolution(tasks, robots, costs);
    double bestMakespan = CalculateMakespan(bestAssignment, robots, costs);
    
    std::cout << "[HillClimbing] Initial greedy makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << "\n";
    
    // Phase 2: Hill climbing with restarts
    for (int restart = 0; restart < maxRestarts_; ++restart) {
        Assignment currentAssignment;
        double currentMakespan;
        
        if (restart == 0) {
            // First iteration uses greedy solution
            currentAssignment = bestAssignment;
            currentMakespan = bestMakespan;
        } else {
            // Random restart
            currentAssignment = GenerateRandomSolution(tasks, robots.size());
            currentMakespan = CalculateMakespan(currentAssignment, robots, costs);
        }
        
        // Local search
        int iterationsWithoutImprovement = 0;
        
        while (iterationsWithoutImprovement < maxIterations_) {
            bool improved = TryImprovement(currentAssignment, robots, costs, currentMakespan);
            
            if (improved) {
                iterationsWithoutImprovement = 0;
                
                // Update best if this is better
                if (currentMakespan < bestMakespan) {
                    bestAssignment = currentAssignment;
                    bestMakespan = currentMakespan;
                }
            } else {
                iterationsWithoutImprovement++;
            }
        }
    }
    
    // Record timing
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    
    std::cout << "[HillClimbing] Final makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << "\n";
    std::cout << "[HillClimbing] Computation time: " 
              << std::fixed << std::setprecision(2) << duration.count() << " ms\n";
    
    // Convert to result format
    result.robotItineraries = AssignmentToItineraries(bestAssignment, robots);
    result.makespan = bestMakespan;
    result.isFeasible = true;
    result.computationTimeMs = duration.count();
    
    // Calculate total distance
    result.totalDistance = 0.0;
    for (size_t i = 0; i < robots.size(); ++i) {
        result.totalDistance += CalculateRobotTime(i, bestAssignment[i], robots[i], costs);
    }
    
    // Assign itineraries to robots
    for (const auto& [robotId, itinerary] : result.robotItineraries) {
        for (auto& robot : robots) {
            if (robot.GetRobotId() == robotId) {
                robot.AssignItinerary(itinerary);
                break;
            }
        }
    }
    
    return result;
}

// =============================================================================
// SOLUTION GENERATION
// =============================================================================

HillClimbing::Assignment HillClimbing::GenerateGreedySolution(
    const std::vector<Task>& tasks,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    int numRobots = static_cast<int>(robots.size());
    Assignment assignment(numRobots);
    
    // Track current end position and time for each robot
    std::vector<int> currentEndNode(numRobots);
    std::vector<double> currentTime(numRobots, 0.0);
    
    for (int i = 0; i < numRobots; ++i) {
        currentEndNode[i] = robots[i].GetCurrentNodeId();
    }
    
    // For each task, find the best robot to assign it to
    for (const auto& task : tasks) {
        int bestRobot = 0;
        double bestAddedCost = std::numeric_limits<double>::max();
        
        for (int r = 0; r < numRobots; ++r) {
            // Cost to pick up the task from current position
            float pickupCost = costs.GetCost(currentEndNode[r], task.sourceNode);
            // Cost to deliver the task
            float deliveryCost = costs.GetCost(task.sourceNode, task.destinationNode);
            
            double addedCost = pickupCost + deliveryCost;
            
            // Prefer robots with lower current load (balance)
            double loadPenalty = assignment[r].size() * 0.1;
            addedCost += loadPenalty;
            
            if (addedCost < bestAddedCost) {
                bestAddedCost = addedCost;
                bestRobot = r;
            }
        }
        
        // Assign task to best robot
        assignment[bestRobot].push_back(task);
        
        // Update robot's end position
        currentEndNode[bestRobot] = task.destinationNode;
    }
    
    return assignment;
}

HillClimbing::Assignment HillClimbing::GenerateRandomSolution(
    const std::vector<Task>& tasks,
    int numRobots
) const {
    Assignment assignment(numRobots);
    
    // Shuffle tasks and distribute round-robin
    std::vector<Task> shuffledTasks = tasks;
    std::shuffle(shuffledTasks.begin(), shuffledTasks.end(), rng_);
    
    for (size_t i = 0; i < shuffledTasks.size(); ++i) {
        assignment[i % numRobots].push_back(shuffledTasks[i]);
    }
    
    return assignment;
}

// =============================================================================
// COST CALCULATION
// =============================================================================

double HillClimbing::CalculateMakespan(
    const Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    double makespan = 0.0;
    
    for (size_t i = 0; i < robots.size(); ++i) {
        double robotTime = CalculateRobotTime(i, assignment[i], robots[i], costs);
        makespan = std::max(makespan, robotTime);
    }
    
    return makespan;
}

double HillClimbing::CalculateRobotTime(
    int robotIdx,
    const std::vector<Task>& robotTasks,
    const RobotAgent& robot,
    const CostMatrixProvider& costs
) const {
    (void)robotIdx; // Unused but kept for interface consistency
    
    if (robotTasks.empty()) return 0.0;
    
    double totalTime = 0.0;
    int currentNode = robot.GetCurrentNodeId();
    
    for (const auto& task : robotTasks) {
        // Travel to pickup
        totalTime += costs.GetCost(currentNode, task.sourceNode);
        // Travel to dropoff
        totalTime += costs.GetCost(task.sourceNode, task.destinationNode);
        // Update position
        currentNode = task.destinationNode;
    }
    
    return totalTime;
}

// =============================================================================
// LOCAL SEARCH MOVES
// =============================================================================

bool HillClimbing::TryImprovement(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    // Try different improvement strategies
    
    // Strategy 1: Inter-robot move (move a task from one robot to another)
    if (TryInterRobotMove(assignment, robots, costs, currentMakespan)) {
        return true;
    }
    
    // Strategy 2: Inter-robot swap (exchange tasks between two robots)
    if (TryInterRobotSwap(assignment, robots, costs, currentMakespan)) {
        return true;
    }
    
    // Strategy 3: Intra-robot reorder (change order of tasks within a robot)
    if (TryIntraRobotReorder(assignment, robots, costs, currentMakespan)) {
        return true;
    }
    
    return false;
}

bool HillClimbing::TryInterRobotMove(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    
    // Find the bottleneck robot (highest completion time)
    int bottleneck = 0;
    double maxTime = 0.0;
    std::vector<double> robotTimes(numRobots);
    
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, assignment[r], robots[r], costs);
        if (robotTimes[r] > maxTime) {
            maxTime = robotTimes[r];
            bottleneck = r;
        }
    }
    
    // If bottleneck has only 1 or 0 tasks, can't move
    if (assignment[bottleneck].size() <= 1) {
        return false;
    }
    
    // Try moving each task from bottleneck to another robot
    for (size_t taskIdx = 0; taskIdx < assignment[bottleneck].size(); ++taskIdx) {
        Task task = assignment[bottleneck][taskIdx];
        
        for (int targetRobot = 0; targetRobot < numRobots; ++targetRobot) {
            if (targetRobot == bottleneck) continue;
            
            // Temporarily move task
            assignment[bottleneck].erase(assignment[bottleneck].begin() + taskIdx);
            assignment[targetRobot].push_back(task);
            
            // Calculate new makespan
            double newMakespan = CalculateMakespan(assignment, robots, costs);
            
            if (newMakespan < currentMakespan) {
                // Keep the improvement
                currentMakespan = newMakespan;
                return true;
            } else {
                // Revert
                assignment[targetRobot].pop_back();
                assignment[bottleneck].insert(assignment[bottleneck].begin() + taskIdx, task);
            }
        }
    }
    
    return false;
}

bool HillClimbing::TryInterRobotSwap(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    
    // Try swapping tasks between random pairs of robots
    std::uniform_int_distribution<int> robotDist(0, numRobots - 1);
    
    for (int attempts = 0; attempts < 10; ++attempts) {
        int r1 = robotDist(rng_);
        int r2 = robotDist(rng_);
        
        if (r1 == r2 || assignment[r1].empty() || assignment[r2].empty()) {
            continue;
        }
        
        // Pick random tasks from each robot
        std::uniform_int_distribution<size_t> task1Dist(0, assignment[r1].size() - 1);
        std::uniform_int_distribution<size_t> task2Dist(0, assignment[r2].size() - 1);
        
        size_t idx1 = task1Dist(rng_);
        size_t idx2 = task2Dist(rng_);
        
        // Swap tasks
        std::swap(assignment[r1][idx1], assignment[r2][idx2]);
        
        // Calculate new makespan
        double newMakespan = CalculateMakespan(assignment, robots, costs);
        
        if (newMakespan < currentMakespan) {
            currentMakespan = newMakespan;
            return true;
        } else {
            // Revert
            std::swap(assignment[r1][idx1], assignment[r2][idx2]);
        }
    }
    
    return false;
}

bool HillClimbing::TryIntraRobotReorder(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    
    // Find the bottleneck robot
    int bottleneck = 0;
    double maxTime = 0.0;
    
    for (int r = 0; r < numRobots; ++r) {
        double robotTime = CalculateRobotTime(r, assignment[r], robots[r], costs);
        if (robotTime > maxTime) {
            maxTime = robotTime;
            bottleneck = r;
        }
    }
    
    // Need at least 2 tasks to reorder
    if (assignment[bottleneck].size() < 2) {
        return false;
    }
    
    // Try 2-opt style swap: reverse a segment of the task list
    std::vector<Task>& tasks = assignment[bottleneck];
    size_t n = tasks.size();
    
    for (size_t i = 0; i < n - 1; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            // Reverse segment [i, j]
            std::reverse(tasks.begin() + i, tasks.begin() + j + 1);
            
            double newMakespan = CalculateMakespan(assignment, robots, costs);
            
            if (newMakespan < currentMakespan) {
                currentMakespan = newMakespan;
                return true;
            } else {
                // Revert
                std::reverse(tasks.begin() + i, tasks.begin() + j + 1);
            }
        }
    }
    
    return false;
}

// =============================================================================
// UTILITY
// =============================================================================

std::map<int, std::vector<int>> HillClimbing::AssignmentToItineraries(
    const Assignment& assignment,
    const std::vector<RobotAgent>& robots
) const {
    std::map<int, std::vector<int>> itineraries;
    
    for (size_t i = 0; i < robots.size(); ++i) {
        int robotId = robots[i].GetRobotId();
        std::vector<int> nodes;
        
        // Expand tasks to pickup/dropoff nodes
        for (const auto& task : assignment[i]) {
            nodes.push_back(task.sourceNode);      // Pickup
            nodes.push_back(task.destinationNode); // Dropoff
        }
        
        itineraries[robotId] = std::move(nodes);
    }
    
    return itineraries;
}

} // namespace Layer2
} // namespace Backend
