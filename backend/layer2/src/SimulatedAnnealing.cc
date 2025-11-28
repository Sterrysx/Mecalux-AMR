/**
 * @file SimulatedAnnealing.cc
 * @brief Implementation of Simulated Annealing VRP solver
 */

#include "../include/SimulatedAnnealing.hh"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <limits>

namespace Backend {
namespace Layer2 {

// =============================================================================
// MAIN SOLVE METHOD
// =============================================================================

VRPResult SimulatedAnnealing::Solve(
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
        std::cout << "[SA] No tasks to assign.\n";
        result.isFeasible = true;
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        for (const auto& robot : robots) {
            result.robotItineraries[robot.GetRobotId()] = {};
        }
        return result;
    }
    
    if (robots.empty()) {
        std::cout << "[SA] No robots available.\n";
        result.isFeasible = false;
        result.makespan = std::numeric_limits<double>::max();
        result.computationTimeMs = 0.0;
        return result;
    }
    
    int numTasks = static_cast<int>(tasks.size());
    int numRobots = static_cast<int>(robots.size());
    
    std::cout << "[SA] Solving VRP: " << numTasks 
              << " tasks, " << numRobots << " robots\n";
    
    // Estimate iterations
    int tempSteps = static_cast<int>(std::log(minTemperature_ / initialTemperature_) / std::log(coolingRate_));
    int totalIterations = tempSteps * iterationsPerTemp_;
    double estimatedMs = totalIterations * 0.01;
    std::cout << "[SA] ETA: ~" << std::fixed << std::setprecision(0) 
              << std::max(1.0, estimatedMs) << " ms (" << tempSteps << " temp steps)\n";
    
    // Phase 1: Generate initial greedy solution
    Assignment currentSolution = GenerateGreedySolution(tasks, robots, costs);
    double currentMakespan = CalculateMakespan(currentSolution, robots, costs);
    
    Assignment bestSolution = currentSolution;
    double bestMakespan = currentMakespan;
    
    std::cout << "[SA] Initial greedy makespan: " 
              << std::fixed << std::setprecision(2) << currentMakespan << " px\n";
    
    // Phase 2: Simulated Annealing loop
    double temperature = initialTemperature_;
    int totalIter = 0;
    int accepted = 0;
    int improved = 0;
    
    while (temperature > minTemperature_) {
        for (int i = 0; i < iterationsPerTemp_; ++i) {
            totalIter++;
            
            // Generate neighbor
            Assignment neighbor = GenerateNeighbor(currentSolution, robots);
            double neighborMakespan = CalculateMakespan(neighbor, robots, costs);
            
            double delta = neighborMakespan - currentMakespan;
            
            // Accept or reject
            bool accept = false;
            if (delta < 0) {
                // Better solution - always accept
                accept = true;
                improved++;
            } else {
                // Worse solution - accept with probability exp(-delta/T)
                double probability = std::exp(-delta / temperature);
                if (uniformDist_(rng_) < probability) {
                    accept = true;
                }
            }
            
            if (accept) {
                accepted++;
                currentSolution = neighbor;
                currentMakespan = neighborMakespan;
                
                // Update best
                if (currentMakespan < bestMakespan) {
                    bestSolution = currentSolution;
                    bestMakespan = currentMakespan;
                }
            }
        }
        
        // Cool down
        temperature *= coolingRate_;
    }
    
    // Record timing
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    
    std::cout << "[SA] Completed: " << totalIter << " iterations, " 
              << accepted << " accepted, " << improved << " improvements\n";
    std::cout << "[SA] Final makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << " px\n";
    std::cout << "[SA] Computation time: " 
              << std::fixed << std::setprecision(2) << duration.count() << " ms\n";
    
    // Convert to result format
    result.robotItineraries = AssignmentToItineraries(bestSolution, robots);
    result.makespan = bestMakespan;
    result.isFeasible = true;
    result.computationTimeMs = duration.count();
    
    // Calculate total distance
    result.totalDistance = 0.0;
    for (size_t i = 0; i < robots.size(); ++i) {
        result.totalDistance += CalculateRobotTime(bestSolution[i], robots[i], costs);
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

SimulatedAnnealing::Assignment SimulatedAnnealing::GenerateGreedySolution(
    const std::vector<Task>& tasks,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    int numRobots = static_cast<int>(robots.size());
    Assignment assignment(numRobots);
    
    std::vector<int> currentEndNode(numRobots);
    std::vector<double> currentTime(numRobots, 0.0);
    
    for (int i = 0; i < numRobots; ++i) {
        currentEndNode[i] = robots[i].GetCurrentNodeId();
    }
    
    for (const auto& task : tasks) {
        int bestRobot = 0;
        double bestFinishTime = std::numeric_limits<double>::max();
        
        for (int r = 0; r < numRobots; ++r) {
            float pickupCost = costs.GetCost(currentEndNode[r], task.sourceNode);
            float deliveryCost = costs.GetCost(task.sourceNode, task.destinationNode);
            double finishTime = currentTime[r] + pickupCost + deliveryCost;
            
            if (finishTime < bestFinishTime) {
                bestFinishTime = finishTime;
                bestRobot = r;
            }
        }
        
        assignment[bestRobot].push_back(task);
        float pickupCost = costs.GetCost(currentEndNode[bestRobot], task.sourceNode);
        float deliveryCost = costs.GetCost(task.sourceNode, task.destinationNode);
        currentTime[bestRobot] += pickupCost + deliveryCost;
        currentEndNode[bestRobot] = task.destinationNode;
    }
    
    return assignment;
}

// =============================================================================
// COST CALCULATION
// =============================================================================

double SimulatedAnnealing::CalculateMakespan(
    const Assignment& assignment,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    double makespan = 0.0;
    
    for (size_t i = 0; i < robots.size(); ++i) {
        double robotTime = CalculateRobotTime(assignment[i], robots[i], costs);
        makespan = std::max(makespan, robotTime);
    }
    
    return makespan;
}

double SimulatedAnnealing::CalculateRobotTime(
    const std::vector<Task>& robotTasks,
    const RobotAgent& robot,
    const CostMatrixProvider& costs
) const {
    if (robotTasks.empty()) return 0.0;
    
    double totalTime = 0.0;
    int currentNode = robot.GetCurrentNodeId();
    
    for (const auto& task : robotTasks) {
        totalTime += costs.GetCost(currentNode, task.sourceNode);
        totalTime += costs.GetCost(task.sourceNode, task.destinationNode);
        currentNode = task.destinationNode;
    }
    
    return totalTime;
}

// =============================================================================
// NEIGHBOR GENERATION
// =============================================================================

SimulatedAnnealing::Assignment SimulatedAnnealing::GenerateNeighbor(
    const Assignment& current,
    const std::vector<RobotAgent>& robots
) const {
    Assignment neighbor = current;
    int numRobots = static_cast<int>(robots.size());
    
    // Choose a random move type
    std::uniform_int_distribution<int> moveTypeDist(0, 2);
    int moveType = moveTypeDist(rng_);
    
    switch (moveType) {
        case 0: {
            // Move: Transfer a task from one robot to another
            std::vector<int> nonEmpty;
            for (int r = 0; r < numRobots; ++r) {
                if (!neighbor[r].empty()) nonEmpty.push_back(r);
            }
            if (nonEmpty.empty()) break;
            
            std::uniform_int_distribution<size_t> srcDist(0, nonEmpty.size() - 1);
            int srcRobot = nonEmpty[srcDist(rng_)];
            
            std::uniform_int_distribution<size_t> taskDist(0, neighbor[srcRobot].size() - 1);
            size_t taskIdx = taskDist(rng_);
            
            std::uniform_int_distribution<int> dstDist(0, numRobots - 1);
            int dstRobot = dstDist(rng_);
            
            if (dstRobot != srcRobot) {
                Task task = neighbor[srcRobot][taskIdx];
                neighbor[srcRobot].erase(neighbor[srcRobot].begin() + taskIdx);
                
                // Insert at random position in destination
                if (neighbor[dstRobot].empty()) {
                    neighbor[dstRobot].push_back(task);
                } else {
                    std::uniform_int_distribution<size_t> posDist(0, neighbor[dstRobot].size());
                    size_t pos = posDist(rng_);
                    neighbor[dstRobot].insert(neighbor[dstRobot].begin() + pos, task);
                }
            }
            break;
        }
        
        case 1: {
            // Swap: Exchange tasks between two robots
            std::vector<int> nonEmpty;
            for (int r = 0; r < numRobots; ++r) {
                if (!neighbor[r].empty()) nonEmpty.push_back(r);
            }
            if (nonEmpty.size() < 2) break;
            
            std::uniform_int_distribution<size_t> robotDist(0, nonEmpty.size() - 1);
            size_t idx1 = robotDist(rng_);
            size_t idx2 = robotDist(rng_);
            while (idx2 == idx1) idx2 = robotDist(rng_);
            
            int r1 = nonEmpty[idx1];
            int r2 = nonEmpty[idx2];
            
            std::uniform_int_distribution<size_t> task1Dist(0, neighbor[r1].size() - 1);
            std::uniform_int_distribution<size_t> task2Dist(0, neighbor[r2].size() - 1);
            
            std::swap(neighbor[r1][task1Dist(rng_)], neighbor[r2][task2Dist(rng_)]);
            break;
        }
        
        case 2: {
            // Reorder: Swap two tasks within the same robot
            std::vector<int> withMultiple;
            for (int r = 0; r < numRobots; ++r) {
                if (neighbor[r].size() >= 2) withMultiple.push_back(r);
            }
            if (withMultiple.empty()) break;
            
            std::uniform_int_distribution<size_t> robotDist(0, withMultiple.size() - 1);
            int robot = withMultiple[robotDist(rng_)];
            
            std::uniform_int_distribution<size_t> taskDist(0, neighbor[robot].size() - 1);
            size_t idx1 = taskDist(rng_);
            size_t idx2 = taskDist(rng_);
            while (idx2 == idx1) idx2 = taskDist(rng_);
            
            std::swap(neighbor[robot][idx1], neighbor[robot][idx2]);
            break;
        }
    }
    
    return neighbor;
}

// =============================================================================
// UTILITY
// =============================================================================

std::map<int, std::vector<int>> SimulatedAnnealing::AssignmentToItineraries(
    const Assignment& assignment,
    const std::vector<RobotAgent>& robots
) const {
    std::map<int, std::vector<int>> itineraries;
    
    for (size_t i = 0; i < robots.size(); ++i) {
        int robotId = robots[i].GetRobotId();
        std::vector<int> nodes;
        
        for (const auto& task : assignment[i]) {
            nodes.push_back(task.sourceNode);
            nodes.push_back(task.destinationNode);
        }
        
        itineraries[robotId] = std::move(nodes);
    }
    
    return itineraries;
}

} // namespace Layer2
} // namespace Backend
