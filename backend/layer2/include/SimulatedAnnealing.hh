/**
 * @file SimulatedAnnealing.hh
 * @brief Simulated Annealing VRP solver implementation
 * 
 * A metaheuristic inspired by the annealing process in metallurgy.
 * Allows uphill moves (worse solutions) with a probability that 
 * decreases as the "temperature" cools down.
 */

#ifndef LAYER2_SIMULATEDANNEALING_HH
#define LAYER2_SIMULATEDANNEALING_HH

#include "IVRPSolver.hh"
#include <random>
#include <algorithm>
#include <cmath>

namespace Backend {
namespace Layer2 {

/**
 * @brief Simulated Annealing algorithm for solving VRP.
 * 
 * Strategy:
 * 1. Generate initial solution using greedy heuristic
 * 2. At each iteration:
 *    - Generate a neighbor solution (random move)
 *    - If better: accept it
 *    - If worse: accept with probability exp(-delta/T)
 * 3. Cool down temperature over time
 * 4. Stop when temperature is too low or max iterations reached
 * 
 * This allows escaping local optima better than Hill Climbing.
 */
class SimulatedAnnealing : public IVRPSolver {
private:
    // Algorithm parameters
    double initialTemperature_;   ///< Starting temperature
    double coolingRate_;          ///< Temperature decay rate (0.9-0.99)
    double minTemperature_;       ///< Stop when T < this
    int iterationsPerTemp_;       ///< Iterations at each temperature
    unsigned int seed_;           ///< Random seed for reproducibility
    
    // Random number generator
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<double> uniformDist_;

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct with tunable parameters.
     * 
     * @param initialTemp Starting temperature (higher = more exploration)
     * @param coolingRate Temperature decay (0.95 = 5% reduction per step)
     * @param minTemp Stop when temperature falls below this
     * @param iterationsPerTemp How many moves to try at each temperature
     * @param seed Random seed (0 = use time)
     */
    explicit SimulatedAnnealing(
        double initialTemp = 1000.0,
        double coolingRate = 0.95,
        double minTemp = 1.0,
        int iterationsPerTemp = 50,
        unsigned int seed = 0
    )
        : initialTemperature_(initialTemp)
        , coolingRate_(coolingRate)
        , minTemperature_(minTemp)
        , iterationsPerTemp_(iterationsPerTemp)
        , seed_(seed)
        , rng_(seed == 0 ? std::random_device{}() : seed)
        , uniformDist_(0.0, 1.0) {}

    // =========================================================================
    // IVRPSOLVER INTERFACE
    // =========================================================================
    
    VRPResult Solve(
        const std::vector<Task>& tasks,
        std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) override;

    std::string GetName() const override { return "Simulated Annealing"; }
    
    std::string GetDescription() const override {
        return "Probabilistic hill climbing that accepts worse solutions early to escape local optima";
    }
    
    bool IsExact() const override { return false; }

private:
    // =========================================================================
    // INTERNAL TYPES
    // =========================================================================
    
    using Assignment = std::vector<std::vector<Task>>;

    // =========================================================================
    // SOLUTION GENERATION
    // =========================================================================
    
    Assignment GenerateGreedySolution(
        const std::vector<Task>& tasks,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) const;

    // =========================================================================
    // COST CALCULATION
    // =========================================================================
    
    double CalculateMakespan(
        const Assignment& assignment,
        const std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) const;

    double CalculateRobotTime(
        const std::vector<Task>& robotTasks,
        const RobotAgent& robot,
        const CostMatrixProvider& costs
    ) const;

    // =========================================================================
    // NEIGHBOR GENERATION
    // =========================================================================
    
    /**
     * @brief Generate a random neighbor solution.
     * 
     * Randomly chooses one of:
     * - Move a task from one robot to another
     * - Swap tasks between two robots
     * - Reorder tasks within a robot
     */
    Assignment GenerateNeighbor(
        const Assignment& current,
        const std::vector<RobotAgent>& robots
    ) const;

    // =========================================================================
    // UTILITY
    // =========================================================================
    
    std::map<int, std::vector<int>> AssignmentToItineraries(
        const Assignment& assignment,
        const std::vector<RobotAgent>& robots
    ) const;
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_SIMULATEDANNEALING_HH
