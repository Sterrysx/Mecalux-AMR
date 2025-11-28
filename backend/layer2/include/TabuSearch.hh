/**
 * @file TabuSearch.hh
 * @brief Tabu Search VRP solver implementation
 * 
 * A metaheuristic that uses memory (tabu list) to prevent cycling
 * back to recently visited solutions, forcing exploration of new areas.
 */

#ifndef LAYER2_TABUSEARCH_HH
#define LAYER2_TABUSEARCH_HH

#include "IVRPSolver.hh"
#include <random>
#include <algorithm>
#include <deque>
#include <unordered_set>

namespace Backend {
namespace Layer2 {

/**
 * @brief Tabu Search algorithm for solving VRP.
 * 
 * Strategy:
 * 1. Generate initial solution using greedy heuristic
 * 2. At each iteration:
 *    - Generate all neighbors (or a sample)
 *    - Select the best non-tabu neighbor (or use aspiration)
 *    - Add the move to the tabu list
 * 3. Stop after max iterations without improvement
 * 
 * Key concepts:
 * - Tabu List: Recently made moves that are forbidden
 * - Aspiration: Accept a tabu move if it's the best ever found
 * - Intensification: Focus on promising regions
 * - Diversification: Escape to unexplored regions
 */
class TabuSearch : public IVRPSolver {
private:
    // Algorithm parameters
    int maxIterations_;           ///< Maximum iterations without improvement
    int tabuTenure_;              ///< How long a move stays tabu
    int neighborhoodSize_;        ///< Number of neighbors to sample per iteration
    unsigned int seed_;           ///< Random seed for reproducibility
    
    // Random number generator
    mutable std::mt19937 rng_;

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct with tunable parameters.
     * 
     * @param maxIterations Max iterations without improvement
     * @param tabuTenure How many iterations a move stays forbidden
     * @param neighborhoodSize How many neighbors to evaluate per iteration
     * @param seed Random seed (0 = use time)
     */
    explicit TabuSearch(
        int maxIterations = 100,
        int tabuTenure = 10,
        int neighborhoodSize = 20,
        unsigned int seed = 0
    )
        : maxIterations_(maxIterations)
        , tabuTenure_(tabuTenure)
        , neighborhoodSize_(neighborhoodSize)
        , seed_(seed)
        , rng_(seed == 0 ? std::random_device{}() : seed) {}

    // =========================================================================
    // IVRPSOLVER INTERFACE
    // =========================================================================
    
    VRPResult Solve(
        const std::vector<Task>& tasks,
        std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) override;

    std::string GetName() const override { return "Tabu Search"; }
    
    std::string GetDescription() const override {
        return "Memory-based search that forbids recently visited solutions to escape local optima";
    }
    
    bool IsExact() const override { return false; }

private:
    // =========================================================================
    // INTERNAL TYPES
    // =========================================================================
    
    using Assignment = std::vector<std::vector<Task>>;
    
    /**
     * @brief Represents a move in the search space.
     */
    struct Move {
        int type;          // 0=transfer, 1=swap, 2=reorder
        int robot1;
        int robot2;
        int taskIdx1;
        int taskIdx2;
        
        bool operator==(const Move& other) const {
            return type == other.type && robot1 == other.robot1 && 
                   robot2 == other.robot2 && taskIdx1 == other.taskIdx1 && 
                   taskIdx2 == other.taskIdx2;
        }
    };
    
    struct MoveHash {
        size_t operator()(const Move& m) const {
            return std::hash<int>()(m.type) ^ 
                   (std::hash<int>()(m.robot1) << 4) ^
                   (std::hash<int>()(m.robot2) << 8) ^
                   (std::hash<int>()(m.taskIdx1) << 12) ^
                   (std::hash<int>()(m.taskIdx2) << 16);
        }
    };

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
     * @brief Generate a set of neighbor solutions.
     */
    std::vector<std::pair<Assignment, Move>> GenerateNeighbors(
        const Assignment& current,
        const std::vector<RobotAgent>& robots,
        int count
    ) const;
    
    /**
     * @brief Apply a move to create a new solution.
     */
    Assignment ApplyMove(
        const Assignment& current,
        const Move& move
    ) const;

    // =========================================================================
    // TABU MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Check if a move is in the tabu list.
     */
    bool IsTabu(const Move& move, const std::deque<Move>& tabuList) const;
    
    /**
     * @brief Get the reverse of a move (for tabu list).
     */
    Move GetReverseMove(const Move& move) const;

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

#endif // LAYER2_TABUSEARCH_HH
