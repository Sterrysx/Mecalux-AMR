/**
 * @file TabuSearch.cc
 * @brief Implementation of Tabu Search VRP solver
 */

#include "../include/TabuSearch.hh"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <limits>

namespace Backend {
namespace Layer2 {

// =============================================================================
// MAIN SOLVE METHOD
// =============================================================================

VRPResult TabuSearch::Solve(
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
        std::cout << "[TS] No tasks to assign.\n";
        result.isFeasible = true;
        result.makespan = 0.0;
        result.computationTimeMs = 0.0;
        for (const auto& robot : robots) {
            result.robotItineraries[robot.GetRobotId()] = {};
        }
        return result;
    }
    
    if (robots.empty()) {
        std::cout << "[TS] No robots available.\n";
        result.isFeasible = false;
        result.makespan = std::numeric_limits<double>::max();
        result.computationTimeMs = 0.0;
        return result;
    }
    
    int numTasks = static_cast<int>(tasks.size());
    int numRobots = static_cast<int>(robots.size());
    
    std::cout << "[TS] Solving VRP: " << numTasks 
              << " tasks, " << numRobots << " robots\n";
    
    // Estimate time
    double estimatedMs = maxIterations_ * neighborhoodSize_ * 0.05;
    std::cout << "[TS] ETA: ~" << std::fixed << std::setprecision(0) 
              << std::max(1.0, estimatedMs) << " ms (tabu tenure=" << tabuTenure_ << ")\n";
    
    // Phase 1: Generate initial greedy solution
    Assignment currentSolution = GenerateGreedySolution(tasks, robots, costs);
    double currentMakespan = CalculateMakespan(currentSolution, robots, costs);
    
    Assignment bestSolution = currentSolution;
    double bestMakespan = currentMakespan;
    
    std::cout << "[TS] Initial greedy makespan: " 
              << std::fixed << std::setprecision(2) << currentMakespan << " px\n";
    
    // Phase 2: Tabu Search loop
    std::deque<Move> tabuList;
    int iterationsWithoutImprovement = 0;
    int totalIterations = 0;
    int improvements = 0;
    
    while (iterationsWithoutImprovement < maxIterations_) {
        totalIterations++;
        
        // Generate neighbors
        auto neighbors = GenerateNeighbors(currentSolution, robots, neighborhoodSize_);
        
        // Find best non-tabu neighbor (or aspiration)
        Assignment bestNeighbor;
        Move bestMove;
        double bestNeighborMakespan = std::numeric_limits<double>::max();
        bool found = false;
        
        for (const auto& [neighbor, move] : neighbors) {
            double neighborMakespan = CalculateMakespan(neighbor, robots, costs);
            
            bool isTabu = IsTabu(move, tabuList);
            
            // Aspiration: accept if it's the best ever
            if (isTabu && neighborMakespan >= bestMakespan) {
                continue;
            }
            
            if (neighborMakespan < bestNeighborMakespan) {
                bestNeighborMakespan = neighborMakespan;
                bestNeighbor = neighbor;
                bestMove = move;
                found = true;
            }
        }
        
        if (!found) {
            // No valid neighbor found, try again
            iterationsWithoutImprovement++;
            continue;
        }
        
        // Move to best neighbor
        currentSolution = bestNeighbor;
        currentMakespan = bestNeighborMakespan;
        
        // Add move to tabu list
        tabuList.push_back(GetReverseMove(bestMove));
        if (static_cast<int>(tabuList.size()) > tabuTenure_) {
            tabuList.pop_front();
        }
        
        // Update best
        if (currentMakespan < bestMakespan) {
            bestSolution = currentSolution;
            bestMakespan = currentMakespan;
            improvements++;
            iterationsWithoutImprovement = 0;
        } else {
            iterationsWithoutImprovement++;
        }
    }
    
    // Record timing
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    
    std::cout << "[TS] Completed: " << totalIterations << " iterations, " 
              << improvements << " improvements\n";
    std::cout << "[TS] Final makespan: " 
              << std::fixed << std::setprecision(2) << bestMakespan << " px\n";
    std::cout << "[TS] Computation time: " 
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

TabuSearch::Assignment TabuSearch::GenerateGreedySolution(
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

double TabuSearch::CalculateMakespan(
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

double TabuSearch::CalculateRobotTime(
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

std::vector<std::pair<TabuSearch::Assignment, TabuSearch::Move>> 
TabuSearch::GenerateNeighbors(
    const Assignment& current,
    const std::vector<RobotAgent>& robots,
    int count
) const {
    std::vector<std::pair<Assignment, Move>> neighbors;
    neighbors.reserve(count);
    
    int numRobots = static_cast<int>(robots.size());
    std::uniform_int_distribution<int> moveTypeDist(0, 2);
    
    for (int i = 0; i < count; ++i) {
        Move move;
        move.type = moveTypeDist(rng_);
        
        switch (move.type) {
            case 0: {
                // Transfer: Move a task from one robot to another
                std::vector<int> nonEmpty;
                for (int r = 0; r < numRobots; ++r) {
                    if (!current[r].empty()) nonEmpty.push_back(r);
                }
                if (nonEmpty.empty()) continue;
                
                std::uniform_int_distribution<size_t> srcDist(0, nonEmpty.size() - 1);
                move.robot1 = nonEmpty[srcDist(rng_)];
                
                std::uniform_int_distribution<size_t> taskDist(0, current[move.robot1].size() - 1);
                move.taskIdx1 = static_cast<int>(taskDist(rng_));
                
                std::uniform_int_distribution<int> dstDist(0, numRobots - 1);
                move.robot2 = dstDist(rng_);
                move.taskIdx2 = static_cast<int>(current[move.robot2].size()); // Append
                
                if (move.robot1 == move.robot2) continue;
                break;
            }
            
            case 1: {
                // Swap: Exchange tasks between two robots
                std::vector<int> nonEmpty;
                for (int r = 0; r < numRobots; ++r) {
                    if (!current[r].empty()) nonEmpty.push_back(r);
                }
                if (nonEmpty.size() < 2) continue;
                
                std::uniform_int_distribution<size_t> robotDist(0, nonEmpty.size() - 1);
                size_t idx1 = robotDist(rng_);
                size_t idx2 = robotDist(rng_);
                while (idx2 == idx1) idx2 = robotDist(rng_);
                
                move.robot1 = nonEmpty[idx1];
                move.robot2 = nonEmpty[idx2];
                
                std::uniform_int_distribution<size_t> task1Dist(0, current[move.robot1].size() - 1);
                std::uniform_int_distribution<size_t> task2Dist(0, current[move.robot2].size() - 1);
                
                move.taskIdx1 = static_cast<int>(task1Dist(rng_));
                move.taskIdx2 = static_cast<int>(task2Dist(rng_));
                break;
            }
            
            case 2: {
                // Reorder: Swap two tasks within the same robot
                std::vector<int> withMultiple;
                for (int r = 0; r < numRobots; ++r) {
                    if (current[r].size() >= 2) withMultiple.push_back(r);
                }
                if (withMultiple.empty()) continue;
                
                std::uniform_int_distribution<size_t> robotDist(0, withMultiple.size() - 1);
                move.robot1 = withMultiple[robotDist(rng_)];
                move.robot2 = move.robot1;
                
                std::uniform_int_distribution<size_t> taskDist(0, current[move.robot1].size() - 1);
                move.taskIdx1 = static_cast<int>(taskDist(rng_));
                move.taskIdx2 = static_cast<int>(taskDist(rng_));
                while (move.taskIdx2 == move.taskIdx1) 
                    move.taskIdx2 = static_cast<int>(taskDist(rng_));
                break;
            }
        }
        
        Assignment neighbor = ApplyMove(current, move);
        neighbors.emplace_back(neighbor, move);
    }
    
    return neighbors;
}

TabuSearch::Assignment TabuSearch::ApplyMove(
    const Assignment& current,
    const Move& move
) const {
    Assignment neighbor = current;
    
    switch (move.type) {
        case 0: {
            // Transfer
            if (move.robot1 >= 0 && move.robot1 < static_cast<int>(neighbor.size()) &&
                move.taskIdx1 >= 0 && move.taskIdx1 < static_cast<int>(neighbor[move.robot1].size())) {
                Task task = neighbor[move.robot1][move.taskIdx1];
                neighbor[move.robot1].erase(neighbor[move.robot1].begin() + move.taskIdx1);
                neighbor[move.robot2].push_back(task);
            }
            break;
        }
        
        case 1: {
            // Swap between robots
            if (move.robot1 >= 0 && move.robot1 < static_cast<int>(neighbor.size()) &&
                move.robot2 >= 0 && move.robot2 < static_cast<int>(neighbor.size()) &&
                move.taskIdx1 >= 0 && move.taskIdx1 < static_cast<int>(neighbor[move.robot1].size()) &&
                move.taskIdx2 >= 0 && move.taskIdx2 < static_cast<int>(neighbor[move.robot2].size())) {
                std::swap(neighbor[move.robot1][move.taskIdx1], 
                          neighbor[move.robot2][move.taskIdx2]);
            }
            break;
        }
        
        case 2: {
            // Reorder within robot
            if (move.robot1 >= 0 && move.robot1 < static_cast<int>(neighbor.size()) &&
                move.taskIdx1 >= 0 && move.taskIdx1 < static_cast<int>(neighbor[move.robot1].size()) &&
                move.taskIdx2 >= 0 && move.taskIdx2 < static_cast<int>(neighbor[move.robot1].size())) {
                std::swap(neighbor[move.robot1][move.taskIdx1], 
                          neighbor[move.robot1][move.taskIdx2]);
            }
            break;
        }
    }
    
    return neighbor;
}

// =============================================================================
// TABU MANAGEMENT
// =============================================================================

bool TabuSearch::IsTabu(const Move& move, const std::deque<Move>& tabuList) const {
    for (const auto& tabuMove : tabuList) {
        if (move == tabuMove) return true;
    }
    return false;
}

TabuSearch::Move TabuSearch::GetReverseMove(const Move& move) const {
    Move reverse = move;
    
    switch (move.type) {
        case 0:
            // Reverse of transfer: transfer back
            std::swap(reverse.robot1, reverse.robot2);
            reverse.taskIdx1 = move.taskIdx2;
            reverse.taskIdx2 = move.taskIdx1;
            break;
        
        case 1:
        case 2:
            // Swap is self-inverse
            break;
    }
    
    return reverse;
}

// =============================================================================
// UTILITY
// =============================================================================

std::map<int, std::vector<int>> TabuSearch::AssignmentToItineraries(
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
