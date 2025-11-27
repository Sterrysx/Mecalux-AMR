/**
 * @file IVRPSolver.hh
 * @brief Interface for VRP (Vehicle Routing Problem) solvers
 * 
 * Defines the contract that all VRP solver implementations must follow.
 * This enables Strategy Pattern for swappable algorithms.
 */

#ifndef LAYER2_IVRPSOLVER_HH
#define LAYER2_IVRPSOLVER_HH

#include "Task.hh"
#include "RobotAgent.hh"
#include "CostMatrixProvider.hh"
#include <vector>
#include <map>
#include <string>
#include <chrono>

namespace Backend {
namespace Layer2 {

/**
 * @brief Result of a VRP solver execution.
 */
struct VRPResult {
    // Robot ID -> Ordered list of goal node IDs (the itinerary)
    std::map<int, std::vector<int>> robotItineraries;
    
    // Performance metrics
    double makespan;            ///< Maximum completion time across all robots
    double totalDistance;       ///< Sum of all robot travel distances
    double computationTimeMs;   ///< Algorithm runtime in milliseconds
    
    // Quality metrics
    bool isFeasible;            ///< Whether all constraints are satisfied
    bool isOptimal;             ///< Whether the solution is guaranteed optimal
    std::string algorithmName;  ///< Name of the algorithm that produced this result
    
    /**
     * @brief Default constructor.
     */
    VRPResult()
        : makespan(0.0)
        , totalDistance(0.0)
        , computationTimeMs(0.0)
        , isFeasible(false)
        , isOptimal(false)
        , algorithmName("Unknown") {}
    
    /**
     * @brief Print result summary.
     */
    void Print() const;
};

/**
 * @brief Abstract interface for VRP solvers (Strategy Pattern).
 * 
 * The VRP (Vehicle Routing Problem) we solve is a variant combining:
 * - Capacitated VRP: Each robot can carry 1 packet at a time
 * - Dynamic VRP: Tasks arrive continuously
 * - Heterogeneous Fleet: Robots have different charging stations
 * 
 * The solver takes:
 * - A list of tasks (pickup/dropoff pairs)
 * - A list of robots (with current positions and states)
 * - A cost matrix provider (for travel costs)
 * 
 * And produces:
 * - An itinerary (ordered list of goal nodes) for each robot
 * 
 * Implementation notes:
 * - Itineraries include BOTH pickup and dropoff nodes
 * - For a Task{source=A, dest=B}, the itinerary will have [..., A, B, ...]
 * - Charging station visits should be inserted automatically if battery runs low
 */
class IVRPSolver {
public:
    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~IVRPSolver() = default;

    /**
     * @brief Solve the VRP and assign tasks to robots.
     * 
     * This is the main entry point. It should:
     * 1. Analyze the current robot states
     * 2. Compute optimal task assignments
     * 3. Generate itineraries for each robot
     * 
     * The returned map contains robot IDs as keys and their assigned
     * itineraries (ordered node IDs) as values.
     * 
     * @param tasks List of tasks to assign
     * @param robots List of available robots (with current states)
     * @param costs Cost matrix provider for travel distances
     * @return VRPResult containing itineraries and metrics
     */
    virtual VRPResult Solve(
        const std::vector<Task>& tasks,
        std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) = 0;

    /**
     * @brief Get the name of the algorithm.
     */
    virtual std::string GetName() const = 0;

    /**
     * @brief Get a brief description of the algorithm.
     */
    virtual std::string GetDescription() const = 0;

    /**
     * @brief Check if the algorithm guarantees optimal solutions.
     */
    virtual bool IsExact() const = 0;

protected:
    /**
     * @brief Helper: Calculate total cost of an itinerary.
     * 
     * @param startNode Starting node (robot's current position)
     * @param itinerary Ordered list of nodes to visit
     * @param costs Cost matrix provider
     * @return Total travel cost
     */
    static float CalculateItineraryCost(
        int startNode,
        const std::vector<int>& itinerary,
        const CostMatrixProvider& costs
    ) {
        if (itinerary.empty()) return 0.0f;
        
        float total = costs.GetCost(startNode, itinerary[0]);
        for (size_t i = 1; i < itinerary.size(); ++i) {
            total += costs.GetCost(itinerary[i-1], itinerary[i]);
        }
        return total;
    }

    /**
     * @brief Helper: Expand tasks into itinerary nodes.
     * 
     * Each task contributes two nodes: source (pickup) and destination (dropoff).
     * 
     * @param tasks Tasks to expand
     * @return Vector of node IDs in order: [P1, D1, P2, D2, ...]
     */
    static std::vector<int> ExpandTasksToNodes(const std::vector<Task>& tasks) {
        std::vector<int> nodes;
        nodes.reserve(tasks.size() * 2);
        for (const auto& task : tasks) {
            nodes.push_back(task.sourceNode);      // Pickup
            nodes.push_back(task.destinationNode); // Dropoff
        }
        return nodes;
    }
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_IVRPSOLVER_HH
