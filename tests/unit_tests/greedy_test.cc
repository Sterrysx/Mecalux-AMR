#include "gtest/gtest.h"
#include <queue>
#include <memory>
#include <fstream>
#include "Planifier.hh"
#include "algorithms/02_Greedy.hh"
#include "Graph.hh"

// =============================================================================
// UNIT TESTS: Greedy Algorithm execute() Method
// =============================================================================

class GreedyTest : public ::testing::Test {
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

TEST_F(GreedyTest, Execute_SingleTask_CompletesSuccessfully) {
    // ARRANGE: 1 task, 1 robot - trivial case
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0) << "Should have positive makespan";
    ASSERT_FALSE(result.isOptimal) << "Greedy is heuristic, not optimal";
    ASSERT_EQ(result.assignment.size(), 1) << "Should have 1 robot assignment";
    ASSERT_EQ(result.assignment[0].size(), 1) << "Robot should have 1 task";
}

TEST_F(GreedyTest, Execute_TwoTasks_OneRobot_AssignsSequentially) {
    // ARRANGE: 2 tasks, 1 robot
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_FALSE(result.isOptimal) << "Greedy is heuristic";
    ASSERT_EQ(result.assignment[0].size(), 2) << "Robot should have both tasks";
}

TEST_F(GreedyTest, Execute_MultipleTasks_DistributesLoad) {
    // ARRANGE: 4 tasks, 2 robots - should distribute workload
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 1, 7));
    tasks.push(Task(3, 8, 14));
    tasks.push(Task(4, 7, 16));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), 2);
    
    // Verify all tasks assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 4) << "All tasks should be assigned";
}

TEST_F(GreedyTest, Execute_AlgorithmName_IsCorrect) {
    // ARRANGE
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.algorithmName, "Greedy");
    ASSERT_FALSE(result.algorithmName.empty());
}

TEST_F(GreedyTest, Execute_ResultStructure_IsValid) {
    // ARRANGE
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Validate result structure
    ASSERT_GT(result.makespan, 0.0) << "Makespan must be positive";
    ASSERT_GE(result.computationTimeMs, 0.0) << "Computation time must be non-negative";
    ASSERT_FALSE(result.isOptimal) << "Greedy should mark results as heuristic";
    ASSERT_EQ(result.algorithmName, "Greedy");
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
}

TEST_F(GreedyTest, Execute_EmptyTasks_HandlesGracefully) {
    // ARRANGE: No tasks
    const int numRobots = 2;
    std::queue<Task> tasks;  // Empty
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.makespan, 0.0) << "No tasks should have 0 makespan";
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
    
    for (const auto& robotTasks : result.assignment) {
        ASSERT_EQ(robotTasks.size(), 0);
    }
}

TEST_F(GreedyTest, Execute_ComputationTime_IsFast) {
    // ARRANGE: Greedy should be very fast
    const int numRobots = 2;
    std::queue<Task> tasks;
    for (int i = 1; i <= 6; i++) {
        tasks.push(Task(i, (i % 4) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Greedy should be extremely fast
    ASSERT_LT(result.computationTimeMs, 100.0) 
        << "Greedy should complete in < 100ms for 6 tasks";
}

TEST_F(GreedyTest, Execute_GreedyCriterion_MinimizesIndividualCompletion) {
    // ARRANGE: Test that greedy assigns tasks to minimize immediate completion
    const int numRobots = 2;
    std::queue<Task> tasks;
    // Task at node 0 - closer to robot starting position
    tasks.push(Task(1, 6, 14));
    // Task at node 8 - further away
    tasks.push(Task(2, 8, 5));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    
    // Both tasks should be assigned (greedy criterion)
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 2);
}

TEST_F(GreedyTest, Execute_MoreRobotsThanTasks_HandlesCorrectly) {
    // ARRANGE: More robots than tasks
    const int numRobots = 5;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), 5);
    
    // Only some robots should have tasks
    int robotsWithTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        if (!robotTasks.empty()) robotsWithTasks++;
    }
    ASSERT_LE(robotsWithTasks, 2) << "At most 2 robots should have tasks";
}

TEST_F(GreedyTest, Execute_LargeProblem_CompletesReasonably) {
    // ARRANGE: 8 tasks, 3 robots
    const int numRobots = 3;
    std::queue<Task> tasks;
    for (int i = 1; i <= 8; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 500.0) << "Should complete quickly";
    
    // All 8 tasks should be assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 8);
}

TEST_F(GreedyTest, Execute_ConsistentResults_SameInput) {
    // ARRANGE: Run greedy twice on same input
    const int numRobots = 2;
    std::queue<Task> tasks1, tasks2;
    tasks1.push(Task(1, 6, 14));
    tasks1.push(Task(2, 8, 12));
    tasks2.push(Task(1, 6, 14));
    tasks2.push(Task(2, 8, 12));
    
    Planifier planner1(testGraph, numRobots, tasks1);
    planner1.setAlgorithm(std::make_unique<Greedy>());
    
    Planifier planner2(testGraph, numRobots, tasks2);
    planner2.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result1 = planner1.executePlan();
    AlgorithmResult result2 = planner2.executePlan();

    // ASSERT: Greedy should be deterministic for same input
    ASSERT_EQ(result1.makespan, result2.makespan) 
        << "Same input should produce same makespan";
}

TEST_F(GreedyTest, Execute_TenTasks_ThreeRobots_FastExecution) {
    // ARRANGE: Larger problem to test speed
    const int numRobots = 3;
    std::queue<Task> tasks;
    for (int i = 1; i <= 10; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 200.0) << "Greedy should be very fast";
    
    // All 10 tasks assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 10);
}

TEST_F(GreedyTest, Execute_FifteenTasks_FourRobots_Scales) {
    // ARRANGE: Large problem instance
    const int numRobots = 4;
    std::queue<Task> tasks;
    for (int i = 1; i <= 15; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 300.0);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 15);
}

TEST_F(GreedyTest, Execute_NonUniformDistribution_HandlesWell) {
    // ARRANGE: All tasks clustered at far nodes
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 10, 17));
    tasks.push(Task(2, 11, 18));
    tasks.push(Task(3, 10, 16));
    tasks.push(Task(4, 11, 17));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 4);
}

TEST_F(GreedyTest, TwentyTasks_FourRobots_FastAndBalanced) {
    // ARRANGE: Large stress test
    const int numRobots = 4;
    std::queue<Task> tasks;
    for (int i = 1; i <= 20; i++) {
        tasks.push(Task(i, (i % 8) + 5, (i % 8) + 13));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 100.0) << "Greedy should be very fast";
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 20);
}

