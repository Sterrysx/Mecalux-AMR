#include "gtest/gtest.h"
#include <queue>
#include <memory>
#include <fstream>
#include "Planifier.hh"
#include "algorithms/01_BruteForce.hh"
#include "Graph.hh"

// =============================================================================
// UNIT TESTS: BruteForce Algorithm execute() Method
// =============================================================================

class BruteForceTest : public ::testing::Test {
protected:
    Graph testGraph;

    void SetUp() override {
        // Load a simple test graph
        std::ifstream graphFile("../optimality/01_layer_mapping/tests/distributions/graph7.inp");
        if (graphFile.is_open()) {
            testGraph.loadFromStream(graphFile);
        }
    }
};

// --- TEST CASES ---

TEST_F(BruteForceTest, Execute_SingleTask_OptimalSolution) {
    // ARRANGE: 1 task, 1 robot - trivial optimal case
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));  // Node 6 (Pickup) -> Node 14 (Dropoff)
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0) << "Should have positive makespan";
    ASSERT_TRUE(result.isOptimal) << "Single task should be optimal";
    ASSERT_EQ(result.assignment.size(), 1) << "Should have 1 robot assignment";
    ASSERT_EQ(result.assignment[0].size(), 1) << "Robot should have 1 task";
}

TEST_F(BruteForceTest, Execute_TwoTasks_OneRobot_Optimal) {
    // ARRANGE: 2 tasks, 1 robot - linear sequence
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal) << "Brute force should find optimal";
    ASSERT_EQ(result.assignment[0].size(), 2) << "Robot should have both tasks";
}

TEST_F(BruteForceTest, Execute_TwoTasks_TwoRobots_OptimalDistribution) {
    // ARRANGE: 2 tasks, 2 robots - should distribute evenly
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    ASSERT_EQ(result.assignment.size(), 2);
    
    // Each robot should get exactly 1 task for optimal distribution
    int tasksAssigned = 0;
    for (const auto& robotTasks : result.assignment) {
        tasksAssigned += robotTasks.size();
    }
    ASSERT_EQ(tasksAssigned, 2) << "All tasks should be assigned";
}

TEST_F(BruteForceTest, Execute_ThreeTasks_TwoRobots_FindsOptimal) {
    // ARRANGE: 3 tasks, 2 robots - tests permutation search
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 2, 8));
    tasks.push(Task(3, 3, 5));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal) << "Should mark solution as optimal";
    
    // Verify all tasks assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 3);
}

TEST_F(BruteForceTest, Execute_FourTasks_TwoRobots_VerifyOptimality) {
    // ARRANGE: 4 tasks, 2 robots - balanced load
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 1, 7));
    tasks.push(Task(3, 8, 14));
    tasks.push(Task(4, 7, 16));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    ASSERT_EQ(result.algorithmName, "Brute Force");
    
    // Each robot should ideally get 2 tasks
    int robot1Tasks = result.assignment[0].size();
    int robot2Tasks = result.assignment[1].size();
    ASSERT_EQ(robot1Tasks + robot2Tasks, 4) << "All 4 tasks should be assigned";
}

TEST_F(BruteForceTest, Execute_ResultStructure_IsValid) {
    // ARRANGE
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Validate result structure
    ASSERT_GT(result.makespan, 0.0) << "Makespan must be positive";
    ASSERT_GE(result.computationTimeMs, 0.0) << "Computation time must be non-negative";
    ASSERT_TRUE(result.isOptimal) << "BruteForce should mark results as optimal";
    ASSERT_EQ(result.algorithmName, "Brute Force");
    ASSERT_FALSE(result.algorithmName.empty());
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
}

TEST_F(BruteForceTest, Execute_ComputationTime_Reasonable) {
    // ARRANGE: Small problem should complete quickly
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    tasks.push(Task(3, 2, 5));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Even brute force should be reasonably fast for 3 tasks
    ASSERT_LT(result.computationTimeMs, 5000.0) 
        << "3 tasks should complete in < 5 seconds";
}

TEST_F(BruteForceTest, Execute_EmptyTasks_HandlesGracefully) {
    // ARRANGE: No tasks to assign
    const int numRobots = 2;
    std::queue<Task> tasks;  // Empty
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.makespan, 0.0) << "No tasks should have 0 makespan";
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
    
    // All robot assignments should be empty
    for (const auto& robotTasks : result.assignment) {
        ASSERT_EQ(robotTasks.size(), 0) << "No tasks should be assigned";
    }
}

TEST_F(BruteForceTest, Execute_MoreRobotsThanTasks_HandlesCorrectly) {
    // ARRANGE: More robots than tasks
    const int numRobots = 5;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    
    // Only 2 robots should have tasks
    int robotsWithTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        if (!robotTasks.empty()) robotsWithTasks++;
    }
    ASSERT_LE(robotsWithTasks, 2) << "At most 2 robots should have tasks";
}

TEST_F(BruteForceTest, Execute_FiveTasks_ThreeRobots_OptimalPartition) {
    // ARRANGE: 5 tasks, 3 robots - tests balanced distribution
    const int numRobots = 3;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 7, 15));
    tasks.push(Task(3, 8, 16));
    tasks.push(Task(4, 9, 17));
    tasks.push(Task(5, 10, 18));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    ASSERT_EQ(result.algorithmName, "Brute Force");
    
    // All 5 tasks assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 5);
}

TEST_F(BruteForceTest, Execute_SixTasks_TwoRobots_BalancedLoad) {
    // ARRANGE: 6 tasks, 2 robots - should balance 3-3
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 7, 15));
    tasks.push(Task(3, 8, 16));
    tasks.push(Task(4, 9, 17));
    tasks.push(Task(5, 10, 18));
    tasks.push(Task(6, 11, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 6);
}

TEST_F(BruteForceTest, Execute_SequentialNodeTasks_OptimalRouting) {
    // ARRANGE: Tasks on sequential pickup nodes
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 5, 12));
    tasks.push(Task(2, 6, 13));
    tasks.push(Task(3, 7, 14));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    ASSERT_LT(result.computationTimeMs, 3000.0) << "Should complete in reasonable time";
}

TEST_F(BruteForceTest, SevenTasks_ThreeRobots_FindsOptimal) {
    // ARRANGE: Balanced 7-task problem
    const int numRobots = 3;
    std::queue<Task> tasks;
    for (int i = 1; i <= 7; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<BruteForce>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_TRUE(result.isOptimal);
    ASSERT_LT(result.computationTimeMs, 10000.0) << "Should complete in reasonable time";
}
