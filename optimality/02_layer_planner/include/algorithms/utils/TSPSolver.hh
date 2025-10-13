#ifndef TSP_SOLVER_HH
#define TSP_SOLVER_HH

#include <vector>
#include "Graph.hh"
#include "Robot.hh"
#include "Task.hh"

class TSPSolver {
public:
    // Finds the minimum time for one robot by checking all task permutations.
    static double findOptimalSequenceTime(
        const Robot& robot,
        const std::vector<Task>& tasks,
        const Graph& graph
    );
};

#endif // TSP_SOLVER_HH
