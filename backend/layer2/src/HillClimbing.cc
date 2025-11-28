/**
 * @file HillClimbing.cc
 * @brief Implementation of Hill Climbing VRP solver
 * 
 * OPTIMIZED for speed - Hill Climbing is a simple local search that
 * will quickly find a local optimum. No need for expensive operations.
 */

#include "../include/HillClimbing.hh"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <limits>
#include <algorithm>
#include <cmath>

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
    
    int numTasks = static_cast<int>(tasks.size());
    int numRobots = static_cast<int>(robots.size());
    
    std::cout << "[HillClimbing] Solving VRP: " << numTasks 
              << " tasks, " << numRobots << " robots\n";
    
    // Estimate time: O(tasks * robots) for greedy + O(restarts * iterations * robots)
    int estimatedOps = numTasks * numRobots + maxRestarts_ * maxIterations_ * numRobots;
    double estimatedMs = estimatedOps * 0.001; // rough estimate
    std::cout << "[HillClimbing] ETA: ~" << std::fixed << std::setprecision(0) 
              << std::max(1.0, estimatedMs) << " ms\n";
    
    // Phase 1: Generate initial greedy solution (fast O(n*k))
    Assignment bestAssignment = GenerateGreedySolution(tasks, robots, costs);
    
    // Pre-compute robot times to avoid recalculating
    std::vector<double> robotTimes(numRobots);
    double bestMakespan = 0.0;
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, bestAssignment[r], robots[r], costs);
        bestMakespan = std::max(bestMakespan, robotTimes[r]);
    }
    
    std::cout << "[HillClimbing] Initial greedy makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << " px\n";
    
    // Phase 2: Fast local search (no restarts for speed, just improve greedy)
    Assignment currentAssignment = bestAssignment;
    std::vector<double> currentTimes = robotTimes;
    double currentMakespan = bestMakespan;
    
    int totalIterations = 0;
    int improvements = 0;
    
    // Simple hill climbing: try to improve until stuck
    for (int restart = 0; restart < maxRestarts_; ++restart) {
        if (restart > 0) {
            // Random restart: shuffle current solution
            currentAssignment = GenerateRandomSolution(tasks, numRobots);
            currentMakespan = 0.0;
            for (int r = 0; r < numRobots; ++r) {
                currentTimes[r] = CalculateRobotTime(r, currentAssignment[r], robots[r], costs);
                currentMakespan = std::max(currentMakespan, currentTimes[r]);
            }
        }
        
        int noImprovement = 0;
        
        while (noImprovement < maxIterations_) {
            totalIterations++;
            
            // Fast improvement: only try moving from bottleneck robot
            bool improved = TryFastImprovement(currentAssignment, currentTimes, 
                                                currentMakespan, robots, costs);
            
            if (improved) {
                improvements++;
                noImprovement = 0;
                
                // Update best
                if (currentMakespan < bestMakespan) {
                    bestAssignment = currentAssignment;
                    bestMakespan = currentMakespan;
                }
            } else {
                noImprovement++;
            }
        }
    }
    
    // Record timing
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    
    std::cout << "[HillClimbing] Completed: " << totalIterations << " iterations, " 
              << improvements << " improvements\n";
    std::cout << "[HillClimbing] Final makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << " px\n";
    std::cout << "[HillClimbing] Computation time: " 
              << std::fixed << std::setprecision(2) << duration.count() << " ms\n";
    
    // Convert to result format
    result.robotItineraries = AssignmentToItineraries(bestAssignment, robots);
    result.makespan = bestMakespan;
    result.isFeasible = true;
    result.computationTimeMs = duration.count();
    
    // Calculate total distance
    result.totalDistance = 0.0;
    for (int i = 0; i < numRobots; ++i) {
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
        double bestFinishTime = std::numeric_limits<double>::max();
        
        for (int r = 0; r < numRobots; ++r) {
            // Cost to pick up the task from current position
            float pickupCost = costs.GetCost(currentEndNode[r], task.sourceNode);
            // Cost to deliver the task
            float deliveryCost = costs.GetCost(task.sourceNode, task.destinationNode);
            
            // When would this robot finish if we assigned this task?
            double finishTime = currentTime[r] + pickupCost + deliveryCost;
            
            if (finishTime < bestFinishTime) {
                bestFinishTime = finishTime;
                bestRobot = r;
            }
        }
        
        // Assign task to best robot
        assignment[bestRobot].push_back(task);
        
        // Update robot's state
        float pickupCost = costs.GetCost(currentEndNode[bestRobot], task.sourceNode);
        float deliveryCost = costs.GetCost(task.sourceNode, task.destinationNode);
        currentTime[bestRobot] += pickupCost + deliveryCost;
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
// FAST LOCAL SEARCH
// =============================================================================

bool HillClimbing::TryFastImprovement(
    Assignment& assignment,
    std::vector<double>& robotTimes,
    double& currentMakespan,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    int numRobots = static_cast<int>(robots.size());
    
    // Find the bottleneck robot (highest completion time)
    int bottleneck = 0;
    for (int r = 1; r < numRobots; ++r) {
        if (robotTimes[r] > robotTimes[bottleneck]) {
            bottleneck = r;
        }
    }
    
    // If bottleneck has only 1 or 0 tasks, try random swap
    if (assignment[bottleneck].size() <= 1) {
        return TryRandomSwap(assignment, robotTimes, currentMakespan, robots, costs);
    }
    
    // Try moving the LAST task from bottleneck (most impactful, O(1) removal)
    Task lastTask = assignment[bottleneck].back();
    
    // Find best target robot for this task
    int bestTarget = -1;
    double bestNewMakespan = currentMakespan;
    
    for (int target = 0; target < numRobots; ++target) {
        if (target == bottleneck) continue;
        
        // Calculate new times if we move the task
        double bottleneckNewTime = CalculateRobotTimeWithout(
            bottleneck, assignment[bottleneck], assignment[bottleneck].size() - 1, 
            robots[bottleneck], costs);
        double targetNewTime = CalculateRobotTimeWithExtra(
            target, assignment[target], lastTask, robots[target], costs);
        
        // New makespan = max of all robot times
        double newMakespan = 0.0;
        for (int r = 0; r < numRobots; ++r) {
            if (r == bottleneck) newMakespan = std::max(newMakespan, bottleneckNewTime);
            else if (r == target) newMakespan = std::max(newMakespan, targetNewTime);
            else newMakespan = std::max(newMakespan, robotTimes[r]);
        }
        
        if (newMakespan < bestNewMakespan) {
            bestNewMakespan = newMakespan;
            bestTarget = target;
        }
    }
    
    if (bestTarget >= 0) {
        // Apply the move
        assignment[bottleneck].pop_back();
        assignment[bestTarget].push_back(lastTask);
        
        // Update times
        robotTimes[bottleneck] = CalculateRobotTime(bottleneck, assignment[bottleneck], 
                                                     robots[bottleneck], costs);
        robotTimes[bestTarget] = CalculateRobotTime(bestTarget, assignment[bestTarget], 
                                                     robots[bestTarget], costs);
        currentMakespan = bestNewMakespan;
        return true;
    }
    
    // If moving last task doesn't help, try a random 2-opt on bottleneck
    return TrySimple2Opt(assignment, robotTimes, currentMakespan, bottleneck, robots, costs);
}

bool HillClimbing::TryRandomSwap(
    Assignment& assignment,
    std::vector<double>& robotTimes,
    double& currentMakespan,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    int numRobots = static_cast<int>(robots.size());
    
    // Pick two random robots with tasks
    std::vector<int> candidates;
    for (int r = 0; r < numRobots; ++r) {
        if (!assignment[r].empty()) candidates.push_back(r);
    }
    
    if (candidates.size() < 2) return false;
    
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    size_t idx1 = dist(rng_);
    size_t idx2 = dist(rng_);
    while (idx2 == idx1) idx2 = dist(rng_);
    
    int r1 = candidates[idx1];
    int r2 = candidates[idx2];
    
    // Swap last tasks
    std::swap(assignment[r1].back(), assignment[r2].back());
    
    // Recalculate
    double newTime1 = CalculateRobotTime(r1, assignment[r1], robots[r1], costs);
    double newTime2 = CalculateRobotTime(r2, assignment[r2], robots[r2], costs);
    
    double newMakespan = 0.0;
    for (int r = 0; r < numRobots; ++r) {
        if (r == r1) newMakespan = std::max(newMakespan, newTime1);
        else if (r == r2) newMakespan = std::max(newMakespan, newTime2);
        else newMakespan = std::max(newMakespan, robotTimes[r]);
    }
    
    if (newMakespan < currentMakespan) {
        robotTimes[r1] = newTime1;
        robotTimes[r2] = newTime2;
        currentMakespan = newMakespan;
        return true;
    } else {
        // Revert
        std::swap(assignment[r1].back(), assignment[r2].back());
        return false;
    }
}

bool HillClimbing::TrySimple2Opt(
    Assignment& assignment,
    std::vector<double>& robotTimes,
    double& currentMakespan,
    int robotIdx,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    auto& tasks = assignment[robotIdx];
    if (tasks.size() < 2) return false;
    
    // Just try swapping adjacent pairs (O(n) instead of O(n²))
    for (size_t i = 0; i < tasks.size() - 1; ++i) {
        std::swap(tasks[i], tasks[i + 1]);
        
        double newTime = CalculateRobotTime(robotIdx, tasks, robots[robotIdx], costs);
        
        if (newTime < robotTimes[robotIdx]) {
            double oldRobotTime = robotTimes[robotIdx];
            robotTimes[robotIdx] = newTime;
            
            // Recalculate makespan only if this was the bottleneck
            if (oldRobotTime >= currentMakespan - 0.001) {
                double newMakespan = 0.0;
                for (size_t r = 0; r < robots.size(); ++r) {
                    newMakespan = std::max(newMakespan, robotTimes[r]);
                }
                if (newMakespan < currentMakespan) {
                    currentMakespan = newMakespan;
                    return true;
                }
            }
            return true;
        } else {
            // Revert
            std::swap(tasks[i], tasks[i + 1]);
        }
    }
    
    return false;
}

double HillClimbing::CalculateRobotTimeWithout(
    int robotIdx,
    const std::vector<Task>& robotTasks,
    size_t excludeIdx,
    const RobotAgent& robot,
    const CostMatrixProvider& costs
) const {
    (void)robotIdx;
    
    double totalTime = 0.0;
    int currentNode = robot.GetCurrentNodeId();
    
    for (size_t i = 0; i < robotTasks.size(); ++i) {
        if (i == excludeIdx) continue;
        
        const auto& task = robotTasks[i];
        totalTime += costs.GetCost(currentNode, task.sourceNode);
        totalTime += costs.GetCost(task.sourceNode, task.destinationNode);
        currentNode = task.destinationNode;
    }
    
    return totalTime;
}

double HillClimbing::CalculateRobotTimeWithExtra(
    int robotIdx,
    const std::vector<Task>& robotTasks,
    const Task& extraTask,
    const RobotAgent& robot,
    const CostMatrixProvider& costs
) const {
    (void)robotIdx;
    
    double totalTime = 0.0;
    int currentNode = robot.GetCurrentNodeId();
    
    for (const auto& task : robotTasks) {
        totalTime += costs.GetCost(currentNode, task.sourceNode);
        totalTime += costs.GetCost(task.sourceNode, task.destinationNode);
        currentNode = task.destinationNode;
    }
    
    // Add extra task at end
    totalTime += costs.GetCost(currentNode, extraTask.sourceNode);
    totalTime += costs.GetCost(extraTask.sourceNode, extraTask.destinationNode);
    
    return totalTime;
}

// =============================================================================
// LEGACY LOCAL SEARCH (kept for compatibility but not used)
// =============================================================================

bool HillClimbing::TryImprovement(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    std::vector<double> robotTimes(numRobots);
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, assignment[r], robots[r], costs);
    }
    
    return TryFastImprovement(const_cast<Assignment&>(assignment), robotTimes, 
                               currentMakespan, robots, costs);
}

bool HillClimbing::TryInterRobotMove(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    // Delegate to fast version
    int numRobots = static_cast<int>(robots.size());
    std::vector<double> robotTimes(numRobots);
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, assignment[r], robots[r], costs);
    }
    return TryFastImprovement(const_cast<Assignment&>(assignment), robotTimes, 
                               currentMakespan, robots, costs);
}

bool HillClimbing::TryInterRobotSwap(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    std::vector<double> robotTimes(numRobots);
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, assignment[r], robots[r], costs);
    }
    return TryRandomSwap(const_cast<Assignment&>(assignment), robotTimes, 
                          currentMakespan, robots, costs);
}

bool HillClimbing::TryIntraRobotReorder(
    Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs,
    double& currentMakespan
) const {
    int numRobots = static_cast<int>(robots.size());
    std::vector<double> robotTimes(numRobots);
    double maxTime = 0.0;
    int bottleneck = 0;
    for (int r = 0; r < numRobots; ++r) {
        robotTimes[r] = CalculateRobotTime(r, assignment[r], robots[r], costs);
        if (robotTimes[r] > maxTime) {
            maxTime = robotTimes[r];
            bottleneck = r;
        }
    }
    return TrySimple2Opt(const_cast<Assignment&>(assignment), robotTimes, 
                          currentMakespan, bottleneck, robots, costs);
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
