/**
 * @file HillClimbing.hh
 * @brief Hill Climbing VRP solver implementation
 * 
 * A metaheuristic that iteratively improves a greedy initial solution
 * through local search (task swaps between robots).
 */

#ifndef LAYER2_HILLCLIMBING_HH
#define LAYER2_HILLCLIMBING_HH

#include "IVRPSolver.hh"
#include <random>
#include <algorithm>

namespace Backend {
namespace Layer2 {

/**
 * @brief Hill Climbing algorithm for solving VRP.
 * 
 * Strategy:
 * 1. Generate initial solution using greedy heuristic
 *    - For each task, assign to the robot that minimizes additional cost
 * 2. Iteratively improve through local search:
 *    - Try swapping tasks between robots
 *    - Try reordering tasks within a robot's itinerary
 *    - Accept changes that reduce makespan
 * 3. Use random restarts to escape local optima
 * 
 * This is a heuristic (not exact), trading optimality for speed.
 */
class HillClimbing : public IVRPSolver {
private:
    // Algorithm parameters
    int maxIterations_;       ///< Maximum iterations without improvement
    int maxRestarts_;         ///< Maximum random restarts
    unsigned int seed_;       ///< Random seed for reproducibility
    
    // Random number generator
    mutable std::mt19937 rng_;

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct with tunable parameters.
     * 
     * @param maxIterations Max iterations without improvement before stopping
     * @param maxRestarts Max random restarts when stuck
     * @param seed Random seed (0 = use time)
     */
    explicit HillClimbing(int maxIterations = 100, int maxRestarts = 5, unsigned int seed = 0)
        : maxIterations_(maxIterations)
        , maxRestarts_(maxRestarts)
        , seed_(seed)
        , rng_(seed == 0 ? std::random_device{}() : seed) {}

    // =========================================================================
    // IVRPSOLVER INTERFACE
    // =========================================================================
    
    /**
     * @brief Solve VRP using Hill Climbing.
     */
    VRPResult Solve(
        const std::vector<Task>& tasks,
        std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) override;

    std::string GetName() const override { return "Hill Climbing"; }
    
    std::string GetDescription() const override {
        return "Greedy initial + local search (inter-robot swaps, intra-robot reordering)";
    }
    
    bool IsExact() const override { return false; }

private:
    // =========================================================================
    // INTERNAL TYPES
    // =========================================================================
    
    /**
     * @brief Internal representation of a solution.
     * 
     * Maps robot index to assigned tasks.
     */
    using Assignment = std::vector<std::vector<Task>>;

    // =========================================================================
    // SOLUTION GENERATION
    // =========================================================================
    
    /**
     * @brief Generate initial greedy solution.
     * 
     * For each task in order:
     * - Compute insertion cost for each robot
     * - Assign to robot with minimum additional cost
     * 
     * @param tasks Tasks to assign
     * @param robots Available robots
     * @param costs Cost matrix
     * @return Initial assignment
     */
    Assignment GenerateGreedySolution(
        const std::vector<Task>& tasks,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) const;

    /**
     * @brief Generate a random solution (for restarts).
     */
    Assignment GenerateRandomSolution(
        const std::vector<Task>& tasks,
        int numRobots
    ) const;

    // =========================================================================
    // COST CALCULATION
    // =========================================================================
    
    /**
     * @brief Calculate makespan of an assignment.
     * 
     * Makespan = maximum completion time across all robots.
     * 
     * @param assignment Current task assignment
     * @param robots Robot list (for starting positions)
     * @param costs Cost matrix
     * @return Makespan value
     */
    double CalculateMakespan(
        const Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) const;

    /**
     * @brief Calculate robot completion time for its assigned tasks.
     */
    double CalculateRobotTime(
        int robotIdx,
        const std::vector<Task>& robotTasks,
        const RobotAgent& robot,
        const CostMatrixProvider& costs
    ) const;

    // =========================================================================
    // LOCAL SEARCH MOVES
    // =========================================================================
    
    /**
     * @brief Try to improve solution through local moves.
     * 
     * @param assignment Current assignment (modified in place if improved)
     * @param robots Robot list
     * @param costs Cost matrix
     * @param currentMakespan Current makespan (updated if improved)
     * @return true if improvement found
     */
    bool TryImprovement(
        Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs,
        double& currentMakespan
    ) const;

    /**
     * @brief Try swapping a task between two robots.
     * 
     * Move a task from robot i to robot j.
     * 
     * @return true if swap improved makespan
     */
    bool TryInterRobotMove(
        Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs,
        double& currentMakespan
    ) const;

    /**
     * @brief Try swapping tasks between two robots.
     * 
     * Exchange a task from robot i with a task from robot j.
     * 
     * @return true if swap improved makespan
     */
    bool TryInterRobotSwap(
        Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs,
        double& currentMakespan
    ) const;

    /**
     * @brief Try reordering tasks within a robot's assignment.
     * 
     * Uses 2-opt style moves on the itinerary.
     * 
     * @return true if reorder improved makespan
     */
    bool TryIntraRobotReorder(
        Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs,
        double& currentMakespan
    ) const;

    // =========================================================================
    // UTILITY
    // =========================================================================
    
    /**
     * @brief Convert assignment to itineraries (expand tasks to nodes).
     */
    std::map<int, std::vector<int>> AssignmentToItineraries(
        const Assignment& assignment,
        const std::vector<RobotAgent>& robots
    ) const;
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_HILLCLIMBING_HH
