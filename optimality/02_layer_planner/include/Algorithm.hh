#ifndef ALGORITHM_HH
#define ALGORITHM_HH

#include <queue>
#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"

struct AlgorithmResult {
    double makespan = 0.0;
    std::vector<std::vector<Task>> assignment;
    double computationTimeMs = 0.0;
    bool isOptimal = false; // True only if the result is guaranteed to be the global optimum.
    std::string algorithmName;
};

/**
 * @brief Abstract base class for planning algorithms (Strategy Pattern)
 * 
 * This interface defines the contract for all planning algorithms.
 * Concrete algorithms inherit from this class and implement the execute() method.
 */
class Algorithm {
public:
    virtual ~Algorithm() = default;

    /**
     * @brief Execute the planning algorithm
     * 
     * @param graph The warehouse graph structure
     * @param availableRobots Queue of robots available for task assignment
     * @param busyRobots Queue of robots currently executing tasks
     * @param chargingRobots Queue of robots charging
     * @param pendingTasks Queue of tasks waiting to be assigned
     * @param totalRobots Total number of robots in the system
     * @param compactMode If true, show only essential metrics (for comparison mode)
     * @return AlgorithmResult containing makespan, assignment, computation time, and optimality info
     */
    virtual AlgorithmResult execute(
        const Graph& graph,
        std::queue<Robot>& availableRobots,
        std::queue<Robot>& busyRobots,
        std::queue<Robot>& chargingRobots,
        std::queue<Task>& pendingTasks,
        int totalRobots,
        bool compactMode = false
    ) = 0;

    /**
     * @brief Get the name of the algorithm
     * @return Algorithm name as string
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get a brief description of the algorithm
     * @return Algorithm description
     */
    virtual std::string getDescription() const = 0;
};

#endif // ALGORITHM_HH
