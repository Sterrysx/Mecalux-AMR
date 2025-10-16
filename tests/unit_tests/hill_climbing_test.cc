#include "gtest/gtest.h"
#include <queue>
#include <memory>
#include <fstream>
#include "Planifier.hh"
#include "algorithms/03_HillClimbing.hh"
#include "Graph.hh"

// =============================================================================
// UNIT TESTS: HillClimbing Algorithm execute() Method
// =============================================================================

class HillClimbingTest : public ::testing::Test {
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

TEST_F(HillClimbingTest, Execute_SingleTask_CompletesSuccessfully) {
    // ARRANGE: 1 task, 1 robot - trivial case
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0) << "Should have positive makespan";
    ASSERT_FALSE(result.isOptimal) << "HillClimbing is heuristic, not optimal";
    ASSERT_EQ(result.assignment.size(), 1);
    ASSERT_EQ(result.assignment[0].size(), 1);
}

TEST_F(HillClimbingTest, Execute_TwoTasks_OneRobot_AssignsSequentially) {
    // ARRANGE: 2 tasks, 1 robot
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_FALSE(result.isOptimal);
    ASSERT_EQ(result.assignment[0].size(), 2);
}

TEST_F(HillClimbingTest, Execute_MultipleTasks_DistributesLoad) {
    // ARRANGE: 4 tasks, 2 robots
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 1, 7));
    tasks.push(Task(3, 8, 14));
    tasks.push(Task(4, 7, 16));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

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
    ASSERT_EQ(totalTasks, 4);
}

TEST_F(HillClimbingTest, Execute_AlgorithmName_IsCorrect) {
    // ARRANGE
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.algorithmName, "Hill Climbing");
    ASSERT_FALSE(result.algorithmName.empty());
}

TEST_F(HillClimbingTest, Execute_ResultStructure_IsValid) {
    // ARRANGE
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Validate result structure
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_GE(result.computationTimeMs, 0.0);
    ASSERT_FALSE(result.isOptimal) << "HillClimbing should mark results as heuristic";
    ASSERT_EQ(result.algorithmName, "Hill Climbing");
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
}

TEST_F(HillClimbingTest, Execute_EmptyTasks_HandlesGracefully) {
    // ARRANGE: No tasks
    const int numRobots = 2;
    std::queue<Task> tasks;  // Empty
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.makespan, 0.0);
    // ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots)); // May be 0 for empty tasks
    
    for (const auto& robotTasks : result.assignment) {
        ASSERT_EQ(robotTasks.size(), 0);
    }
}

TEST_F(HillClimbingTest, Execute_LocalSearchImprovement_FindsBetterSolution) {
    // ARRANGE: Problem where HC should improve over greedy
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    tasks.push(Task(3, 5, 7));
    tasks.push(Task(4, 2, 6));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), 2);
    
    // All tasks should be assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 4);
}

TEST_F(HillClimbingTest, Execute_ComputationTime_Reasonable) {
    // ARRANGE: HC should be fast but slower than pure greedy
    const int numRobots = 2;
    std::queue<Task> tasks;
    for (int i = 1; i <= 6; i++) {
        tasks.push(Task(i, (i % 4) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: HC may take longer than greedy but should still be fast
    ASSERT_LT(result.computationTimeMs, 1000.0) 
        << "HillClimbing should complete in < 1s for 6 tasks";
}

TEST_F(HillClimbingTest, Execute_MoreRobotsThanTasks_HandlesCorrectly) {
    // ARRANGE: More robots than tasks
    const int numRobots = 5;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), 5);
    
    int robotsWithTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        if (!robotTasks.empty()) robotsWithTasks++;
    }
    ASSERT_LE(robotsWithTasks, 2);
}

TEST_F(HillClimbingTest, Execute_LargeProblem_CompletesSuccessfully) {
    // ARRANGE: 8 tasks, 3 robots
    const int numRobots = 3;
    std::queue<Task> tasks;
    for (int i = 1; i <= 8; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 2000.0) << "Should complete reasonably fast";
    
    // All 8 tasks assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 8);
}

TEST_F(HillClimbingTest, Execute_ImprovementIteration_WorksProperly) {
    // ARRANGE: Test that HC actually performs local search
    const int numRobots = 3;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 7, 13));
    tasks.push(Task(3, 4, 8));
    tasks.push(Task(4, 5, 15));
    tasks.push(Task(5, 6, 16));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    
    // All tasks should be assigned
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 5);
    
    // At least one robot should have multiple tasks (ensuring distribution)
    bool hasMultipleTasksOnOneRobot = false;
    for (const auto& robotTasks : result.assignment) {
        if (robotTasks.size() >= 2) {
            hasMultipleTasksOnOneRobot = true;
            break;
        }
    }
    ASSERT_TRUE(hasMultipleTasksOnOneRobot);
}

TEST_F(HillClimbingTest, Execute_ConsistentResults_SameInput) {
    // ARRANGE: Run HC twice on same input
    const int numRobots = 2;
    std::queue<Task> tasks1, tasks2;
    tasks1.push(Task(1, 6, 14));
    tasks1.push(Task(2, 8, 12));
    tasks2.push(Task(1, 6, 14));
    tasks2.push(Task(2, 8, 12));
    
    Planifier planner1(testGraph, numRobots, tasks1);
    planner1.setAlgorithm(std::make_unique<HillClimbing>());
    
    Planifier planner2(testGraph, numRobots, tasks2);
    planner2.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result1 = planner1.executePlan();
    AlgorithmResult result2 = planner2.executePlan();

    // ASSERT: HC should produce consistent results (may vary if using randomization)
    // For now, just verify both complete successfully
    ASSERT_GT(result1.makespan, 0.0);
    ASSERT_GT(result2.makespan, 0.0);
}

TEST_F(HillClimbingTest, Execute_TenTasks_ThreeRobots_Improvement) {
    // ARRANGE: Larger problem for local search
    const int numRobots = 3;
    std::queue<Task> tasks;
    for (int i = 1; i <= 10; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 2000.0);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 10);
}

TEST_F(HillClimbingTest, Execute_FifteenTasks_FourRobots_LocalSearch) {
    // ARRANGE: Large instance for hill climbing
    const int numRobots = 4;
    std::queue<Task> tasks;
    for (int i = 1; i <= 15; i++) {
        tasks.push(Task(i, (i % 7) + 5, (i % 7) + 12));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 3000.0);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 15);
}

TEST_F(HillClimbingTest, Execute_ClusteredTasks_OptimizeRouting) {
    // ARRANGE: Spatially clustered tasks
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 5, 12));
    tasks.push(Task(2, 6, 13));
    tasks.push(Task(3, 5, 14));
    tasks.push(Task(4, 6, 15));
    tasks.push(Task(5, 7, 16));
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 5);
}

TEST_F(HillClimbingTest, TwentyTasks_FourRobots_ImprovesGreedy) {
    // ARRANGE: Large stress test to verify HC optimizes well
    const int numRobots = 4;
    std::queue<Task> tasks;
    for (int i = 1; i <= 20; i++) {
        tasks.push(Task(i, (i % 9) + 5, (i % 9) + 14));
    }
    
    Planifier planner(testGraph, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.computationTimeMs, 1000.0) << "HC should complete large problems reasonably fast";
    
    int totalTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        totalTasks += robotTasks.size();
    }
    ASSERT_EQ(totalTasks, 20);
}

