/**
 * @file ALNS.cc
 * @brief Implementation of Adaptive Large Neighborhood Search VRP solver
 * 
 * This implements the "Destroy and Repair" paradigm:
 * - Destroy: Remove 20-30% of tasks (worst removal - remove expensive outliers)
 * - Repair: Re-insert using Regret-2 heuristic (prioritize tasks with high regret)
 */

#include "ALNS.hh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace Backend {
namespace Layer2 {

// =============================================================================
// MAIN SOLVE METHOD
// =============================================================================

VRPResult ALNS::Solve(
    const std::vector<Task>& tasks,
    std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    VRPResult result;
    result.algorithmName = GetName();
    
    // Handle edge cases
    if (tasks.empty()) {
        result.isFeasible = true;
        result.makespan = 0;
        result.totalDistance = 0;
        for (const auto& robot : robots) {
            result.robotItineraries[robot.GetRobotId()] = {};
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        result.computationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return result;
    }
    
    if (robots.empty()) {
        result.isFeasible = false;
        result.algorithmName = GetName() + " (No robots available)";
        return result;
    }
    
    // Print algorithm info
    std::cout << "[ALNS] Solving VRP: " << tasks.size() << " tasks, " << robots.size() << " robots\n";
    std::cout << "[ALNS] Parameters: " << maxIterations_ << " iterations, "
              << (destructionFactor_ * 100) << "% destruction\n";
    
    // 1. Generate initial solution (Round-Robin)
    Solution currentSol = GenerateInitialSolution(tasks, robots.size());
    double currentCost = CalculateSolutionCost(currentSol, robots, costs);
    
    // Track best solution
    Solution bestSol = currentSol;
    double bestCost = currentCost;
    
    std::cout << "[ALNS] Initial greedy makespan: " << std::fixed << std::setprecision(2) 
              << currentCost << " px\n";
    
    // Calculate number of tasks to remove each iteration
    int numToRemove = std::max(1, static_cast<int>(tasks.size() * destructionFactor_));
    
    // Statistics
    int improvements = 0;
    int worstRemovals = 0;
    int randomRemovals = 0;
    
    // 2. Main ALNS loop
    for (int iter = 0; iter < maxIterations_; ++iter) {
        // Create a copy to work with
        Solution tempSol = currentSol;
        std::vector<Task> unassigned;
        
        // A. DESTROY phase - alternate between worst and random removal
        if (iter % 3 != 0) {
            // Worst removal (2/3 of iterations)
            DestroyWorst(tempSol, unassigned, numToRemove, robots, costs);
            worstRemovals++;
        } else {
            // Random removal (1/3 of iterations for diversity)
            DestroyRandom(tempSol, unassigned, numToRemove);
            randomRemovals++;
        }
        
        // B. REPAIR phase - use Regret-2 insertion
        RepairRegret(tempSol, unassigned, robots, costs);
        
        // C. EVALUATE - calculate new cost
        double newCost = CalculateSolutionCost(tempSol, robots, costs);
        
        // D. ACCEPTANCE - greedy (accept if better)
        if (newCost < currentCost) {
            currentSol = tempSol;
            currentCost = newCost;
            
            // Update best if this is a new global best
            if (newCost < bestCost) {
                bestSol = tempSol;
                bestCost = newCost;
                improvements++;
            }
        }
        
        // Optional: Add Simulated Annealing acceptance for even better exploration
        // (Accept worse solutions with decreasing probability)
        // double temp = 1000.0 * exp(-5.0 * iter / maxIterations_);
        // if (newCost >= currentCost && exp((currentCost - newCost) / temp) > uniform(rng_)) {
        //     currentSol = tempSol;
        //     currentCost = newCost;
        // }
    }
    
    // 3. Format output
    result.robotItineraries = FormatResult(bestSol, robots);
    result.makespan = bestCost;
    result.totalDistance = CalculateTotalDistance(bestSol, robots, costs);
    result.isFeasible = true;
    result.isOptimal = false;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.computationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[ALNS] Completed: " << maxIterations_ << " iterations, " 
              << improvements << " improvements\n";
    std::cout << "[ALNS] Destroy stats: " << worstRemovals << " worst, " 
              << randomRemovals << " random\n";
    std::cout << "[ALNS] Final makespan: " << std::fixed << std::setprecision(2) 
              << bestCost << " px\n";
    std::cout << "[ALNS] Computation time: " << std::fixed << std::setprecision(2) 
              << result.computationTimeMs << " ms\n";
    
    return result;
}

// =============================================================================
// DESTROY OPERATORS
// =============================================================================

void ALNS::DestroyWorst(
    Solution& sol,
    std::vector<Task>& unassigned,
    int count,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    // Collect all tasks with their cost contributions
    std::vector<TaskCost> taskCosts;
    
    for (size_t r = 0; r < sol.size(); ++r) {
        if (sol[r].empty()) continue;
        
        int startNode = robots[r].GetCurrentNodeId();
        
        for (size_t t = 0; t < sol[r].size(); ++t) {
            double savings = CalculateRemovalSavings(sol[r], static_cast<int>(t), startNode, costs);
            taskCosts.push_back({static_cast<int>(r), static_cast<int>(t), savings});
        }
    }
    
    // Sort by cost (descending - worst first)
    std::sort(taskCosts.begin(), taskCosts.end(), 
              [](const TaskCost& a, const TaskCost& b) { return a.cost > b.cost; });
    
    // Remove the worst tasks (need to handle indices carefully)
    // We'll mark tasks to remove, then remove them in reverse order
    std::vector<std::pair<int, int>> toRemove; // (robot, taskIndex)
    
    for (int i = 0; i < std::min(count, static_cast<int>(taskCosts.size())); ++i) {
        toRemove.push_back({taskCosts[i].robotIndex, taskCosts[i].taskIndex});
    }
    
    // Sort by (robot, taskIndex) descending so we can remove from back to front
    std::sort(toRemove.begin(), toRemove.end(), 
              [](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                  if (a.first != b.first) return a.first > b.first;
                  return a.second > b.second;
              });
    
    // Remove tasks (from back to front within each robot to preserve indices)
    for (const auto& [robotIdx, taskIdx] : toRemove) {
        unassigned.push_back(sol[robotIdx][taskIdx]);
        sol[robotIdx].erase(sol[robotIdx].begin() + taskIdx);
    }
}

void ALNS::DestroyRandom(
    Solution& sol,
    std::vector<Task>& unassigned,
    int count
) const {
    // Collect all (robot, taskIndex) pairs
    std::vector<std::pair<int, int>> allTasks;
    
    for (size_t r = 0; r < sol.size(); ++r) {
        for (size_t t = 0; t < sol[r].size(); ++t) {
            allTasks.push_back({static_cast<int>(r), static_cast<int>(t)});
        }
    }
    
    // Shuffle and pick first `count`
    std::shuffle(allTasks.begin(), allTasks.end(), rng_);
    
    // Sort selected tasks by (robot, taskIndex) descending for safe removal
    int numToRemove = std::min(count, static_cast<int>(allTasks.size()));
    std::vector<std::pair<int, int>> toRemove(allTasks.begin(), allTasks.begin() + numToRemove);
    
    std::sort(toRemove.begin(), toRemove.end(),
              [](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                  if (a.first != b.first) return a.first > b.first;
                  return a.second > b.second;
              });
    
    // Remove tasks
    for (const auto& [robotIdx, taskIdx] : toRemove) {
        unassigned.push_back(sol[robotIdx][taskIdx]);
        sol[robotIdx].erase(sol[robotIdx].begin() + taskIdx);
    }
}

// =============================================================================
// REPAIR OPERATORS
// =============================================================================

void ALNS::RepairRegret(
    Solution& sol,
    std::vector<Task>& unassigned,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    while (!unassigned.empty()) {
        int bestTaskIdx = -1;
        double maxRegret = -std::numeric_limits<double>::max();
        
        // Store the best move for each task
        std::vector<InsertionMove> bestMoves(unassigned.size());
        
        // For each unassigned task, find best and 2nd best insertion positions
        for (size_t t = 0; t < unassigned.size(); ++t) {
            const Task& task = unassigned[t];
            
            // Collect all valid insertion positions
            std::vector<InsertionMove> validMoves;
            
            // Try every robot, every position
            for (size_t r = 0; r < sol.size(); ++r) {
                int startNode = robots[r].GetCurrentNodeId();
                
                // Can insert at positions 0 to route.size() (inclusive)
                for (size_t p = 0; p <= sol[r].size(); ++p) {
                    double insertCost = CalculateInsertionCost(
                        sol[r], task, static_cast<int>(p), startNode, costs
                    );
                    validMoves.emplace_back(static_cast<int>(r), static_cast<int>(p), insertCost);
                }
            }
            
            // Sort by insertion cost (ascending - cheapest first)
            std::sort(validMoves.begin(), validMoves.end(),
                      [](const InsertionMove& a, const InsertionMove& b) {
                          return a.insertionCost < b.insertionCost;
                      });
            
            // Calculate regret = 2nd best - best
            double regret;
            if (validMoves.size() >= 2) {
                regret = validMoves[1].insertionCost - validMoves[0].insertionCost;
            } else if (validMoves.size() == 1) {
                // Only one option - must prioritize
                regret = std::numeric_limits<double>::max();
            } else {
                // No valid moves (shouldn't happen)
                regret = 0;
            }
            
            // Store best move for this task
            if (!validMoves.empty()) {
                bestMoves[t] = validMoves[0];
            }
            
            // Track task with highest regret
            if (regret > maxRegret) {
                maxRegret = regret;
                bestTaskIdx = static_cast<int>(t);
            }
        }
        
        // Insert the task with highest regret at its best position
        if (bestTaskIdx >= 0 && bestMoves[bestTaskIdx].robotIndex >= 0) {
            const InsertionMove& move = bestMoves[bestTaskIdx];
            sol[move.robotIndex].insert(
                sol[move.robotIndex].begin() + move.position,
                unassigned[bestTaskIdx]
            );
            unassigned.erase(unassigned.begin() + bestTaskIdx);
        } else {
            // Fallback: assign to robot with shortest route
            if (!unassigned.empty()) {
                size_t minRobot = 0;
                size_t minSize = sol[0].size();
                for (size_t r = 1; r < sol.size(); ++r) {
                    if (sol[r].size() < minSize) {
                        minSize = sol[r].size();
                        minRobot = r;
                    }
                }
                sol[minRobot].push_back(unassigned.back());
                unassigned.pop_back();
            }
        }
    }
}

void ALNS::RepairGreedy(
    Solution& sol,
    std::vector<Task>& unassigned,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    while (!unassigned.empty()) {
        const Task& task = unassigned.back();
        
        InsertionMove bestMove;
        
        // Find cheapest insertion across all robots and positions
        for (size_t r = 0; r < sol.size(); ++r) {
            int startNode = robots[r].GetCurrentNodeId();
            
            for (size_t p = 0; p <= sol[r].size(); ++p) {
                double insertCost = CalculateInsertionCost(
                    sol[r], task, static_cast<int>(p), startNode, costs
                );
                
                if (insertCost < bestMove.insertionCost) {
                    bestMove = InsertionMove(static_cast<int>(r), static_cast<int>(p), insertCost);
                }
            }
        }
        
        // Insert at best position
        if (bestMove.robotIndex >= 0) {
            sol[bestMove.robotIndex].insert(
                sol[bestMove.robotIndex].begin() + bestMove.position,
                task
            );
        } else {
            // Fallback: assign to first robot
            sol[0].push_back(task);
        }
        
        unassigned.pop_back();
    }
}

// =============================================================================
// COST CALCULATIONS
// =============================================================================

double ALNS::CalculateSolutionCost(
    const Solution& sol,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    // Makespan = maximum route cost across all robots
    double makespan = 0;
    
    for (size_t r = 0; r < sol.size(); ++r) {
        int startNode = robots[r].GetCurrentNodeId();
        double routeCost = CalculateRouteCost(sol[r], startNode, costs);
        makespan = std::max(makespan, routeCost);
    }
    
    return makespan;
}

double ALNS::CalculateTotalDistance(
    const Solution& sol,
    const std::vector<RobotAgent>& robots,
    const CostMatrixProvider& costs
) const {
    double total = 0;
    
    for (size_t r = 0; r < sol.size(); ++r) {
        int startNode = robots[r].GetCurrentNodeId();
        total += CalculateRouteCost(sol[r], startNode, costs);
    }
    
    return total;
}

double ALNS::CalculateRouteCost(
    const std::vector<Task>& route,
    int startNode,
    const CostMatrixProvider& costs
) const {
    if (route.empty()) return 0;
    
    double total = 0;
    int prevNode = startNode;
    
    for (const Task& task : route) {
        // Cost to pickup
        total += costs.GetCost(prevNode, task.sourceNode);
        // Cost from pickup to dropoff
        total += costs.GetCost(task.sourceNode, task.destinationNode);
        // Update prev to dropoff
        prevNode = task.destinationNode;
    }
    
    return total;
}

double ALNS::CalculateInsertionCost(
    const std::vector<Task>& route,
    const Task& task,
    int position,
    int startNode,
    const CostMatrixProvider& costs
) const {
    // Get the previous and next nodes
    int prevNode, nextNode;
    double oldCost = 0;
    
    if (route.empty()) {
        // Inserting into empty route
        prevNode = startNode;
        nextNode = -1; // No next
    } else if (position == 0) {
        // Inserting at front
        prevNode = startNode;
        nextNode = route[0].sourceNode;
        oldCost = costs.GetCost(prevNode, nextNode);
    } else if (position >= static_cast<int>(route.size())) {
        // Inserting at back
        prevNode = route.back().destinationNode;
        nextNode = -1; // No next
    } else {
        // Inserting in middle
        prevNode = route[position - 1].destinationNode;
        nextNode = route[position].sourceNode;
        oldCost = costs.GetCost(prevNode, nextNode);
    }
    
    // Calculate new cost with task inserted
    double newCost = costs.GetCost(prevNode, task.sourceNode) 
                   + costs.GetCost(task.sourceNode, task.destinationNode);
    
    if (nextNode >= 0) {
        newCost += costs.GetCost(task.destinationNode, nextNode);
    }
    
    return newCost - oldCost;
}

double ALNS::CalculateRemovalSavings(
    const std::vector<Task>& route,
    int taskIndex,
    int startNode,
    const CostMatrixProvider& costs
) const {
    if (route.empty() || taskIndex < 0 || taskIndex >= static_cast<int>(route.size())) {
        return 0;
    }
    
    const Task& task = route[taskIndex];
    
    // Get previous and next nodes
    int prevNode, nextNode;
    
    if (taskIndex == 0) {
        prevNode = startNode;
    } else {
        prevNode = route[taskIndex - 1].destinationNode;
    }
    
    if (taskIndex == static_cast<int>(route.size()) - 1) {
        nextNode = -1; // No next
    } else {
        nextNode = route[taskIndex + 1].sourceNode;
    }
    
    // Current cost with task
    double currentCost = costs.GetCost(prevNode, task.sourceNode)
                       + costs.GetCost(task.sourceNode, task.destinationNode);
    
    if (nextNode >= 0) {
        currentCost += costs.GetCost(task.destinationNode, nextNode);
    }
    
    // Cost without task
    double costWithout = 0;
    if (nextNode >= 0) {
        costWithout = costs.GetCost(prevNode, nextNode);
    }
    
    // Savings = current cost - cost without = what we "save" by removing
    return currentCost - costWithout;
}

// =============================================================================
// OUTPUT FORMATTING
// =============================================================================

std::map<int, std::vector<int>> ALNS::FormatResult(
    const Solution& sol,
    const std::vector<RobotAgent>& robots
) const {
    std::map<int, std::vector<int>> result;
    
    for (size_t r = 0; r < sol.size(); ++r) {
        int robotId = robots[r].GetRobotId();
        std::vector<int> itinerary;
        
        // Expand each task to pickup + dropoff nodes
        for (const Task& task : sol[r]) {
            itinerary.push_back(task.sourceNode);      // Pickup
            itinerary.push_back(task.destinationNode); // Dropoff
        }
        
        result[robotId] = itinerary;
    }
    
    return result;
}

// =============================================================================
// INITIAL SOLUTION
// =============================================================================

ALNS::Solution ALNS::GenerateInitialSolution(
    const std::vector<Task>& tasks,
    size_t numRobots
) const {
    Solution sol(numRobots);
    
    // Round-robin assignment
    for (size_t i = 0; i < tasks.size(); ++i) {
        sol[i % numRobots].push_back(tasks[i]);
    }
    
    return sol;
}

} // namespace Layer2
} // namespace Backend
