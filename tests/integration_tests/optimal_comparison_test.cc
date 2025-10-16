#include "gtest/gtest.h"
#include <queue>
#include <memory>
#include <cmath>
#include <fstream>
#include "Planifier.hh"
#include "algorithms/01_BruteForce.hh"
#include "algorithms/02_Greedy.hh"
#include "algorithms/03_HillClimbing.hh"
#include "Graph.hh"

// =============================================================================
// INTEGRATION TESTS: Heuristics vs BruteForce Optimal Comparison
// =============================================================================
// Purpose: Verify that heuristic algorithms (Greedy, HillClimbing) produce
//          solutions that are close to optimal (BruteForce) solutions
// =============================================================================

class OptimalComparisonTest : public ::testing::Test {
protected:
    Graph testGraph;

    void SetUp() override {
        std::ifstream graphFile("../optimality/01_layer_mapping/tests/distributions/graph7.inp");
        if (graphFile.is_open()) {
            testGraph.loadFromStream(graphFile);
        }
    }

    // Helper: Run all three algorithms and return results
    struct ComparisonResult {
        double bruteForce_makespan;
        double greedy_makespan;
        double hillClimbing_makespan;
        double greedy_deviation_percent;      // (greedy - optimal) / optimal * 100
        double hillClimbing_deviation_percent; // (HC - optimal) / optimal * 100
    };

    ComparisonResult runComparison(int numRobots, std::vector<Task> taskList) {
        ComparisonResult result;

        // Run BruteForce (optimal)
        std::queue<Task> tasks1;
        for (const auto& t : taskList) tasks1.push(t);
        Planifier planner1(testGraph, numRobots, tasks1);
        planner1.setAlgorithm(std::make_unique<BruteForce>());
        AlgorithmResult bf_result = planner1.executePlan();
        result.bruteForce_makespan = bf_result.makespan;

        // Run Greedy
        std::queue<Task> tasks2;
        for (const auto& t : taskList) tasks2.push(t);
        Planifier planner2(testGraph, numRobots, tasks2);
        planner2.setAlgorithm(std::make_unique<Greedy>());
        AlgorithmResult greedy_result = planner2.executePlan();
        result.greedy_makespan = greedy_result.makespan;

        // Run HillClimbing
        std::queue<Task> tasks3;
        for (const auto& t : taskList) tasks3.push(t);
        Planifier planner3(testGraph, numRobots, tasks3);
        planner3.setAlgorithm(std::make_unique<HillClimbing>());
        AlgorithmResult hc_result = planner3.executePlan();
        result.hillClimbing_makespan = hc_result.makespan;

        // Calculate deviations
        if (result.bruteForce_makespan > 0) {
            result.greedy_deviation_percent = 
                ((result.greedy_makespan - result.bruteForce_makespan) / result.bruteForce_makespan) * 100.0;
            result.hillClimbing_deviation_percent = 
                ((result.hillClimbing_makespan - result.bruteForce_makespan) / result.bruteForce_makespan) * 100.0;
        } else {
            result.greedy_deviation_percent = 0.0;
            result.hillClimbing_deviation_percent = 0.0;
        }

        return result;
    }
};

// --- TEST CASES ---

TEST_F(OptimalComparisonTest, SmallProblem_2Tasks2Robots_CloseToOptimal) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14),
        Task(2, 8, 12)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: Heuristics should be within 20% of optimal for small problems
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 20.0) 
        << "Greedy deviation: " << result.greedy_deviation_percent << "%";
    ASSERT_LE(result.hillClimbing_deviation_percent, 20.0)
        << "HillClimbing deviation: " << result.hillClimbing_deviation_percent << "%";
}

TEST_F(OptimalComparisonTest, MediumProblem_4Tasks2Robots_CloseToOptimal) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14),
        Task(2, 1, 7),
        Task(3, 8, 14),
        Task(4, 7, 16)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 25.0)
        << "Greedy should be within 25% of optimal";
    ASSERT_LE(result.hillClimbing_deviation_percent, 20.0)
        << "HC should be within 20% of optimal";
}

TEST_F(OptimalComparisonTest, LargeProblem_8Tasks2Robots_ReasonableQuality) {
    // ARRANGE: 8 tasks, 2 robots - BruteForce may take time
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14), Task(2, 1, 7), Task(3, 8, 14), Task(4, 7, 16),
        Task(5, 6, 13), Task(6, 9, 16), Task(7, 6, 18), Task(8, 10, 12)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: Larger problems may have higher deviation
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 35.0)
        << "Greedy should be within 35% for larger problems";
    ASSERT_LE(result.hillClimbing_deviation_percent, 30.0)
        << "HC should be within 30% for larger problems";
}

TEST_F(OptimalComparisonTest, UnbalancedLoad_3Tasks3Robots_CloseToOptimal) {
    // ARRANGE: More robots than needed
    const int numRobots = 3;
    std::vector<Task> tasks = {
        Task(1, 6, 14),
        Task(2, 2, 8),
        Task(3, 7, 14)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 15.0)
        << "With abundant robots, greedy should be very close";
    ASSERT_LE(result.hillClimbing_deviation_percent, 10.0);
}

TEST_F(OptimalComparisonTest, HighLoad_6Tasks2Robots_AcceptableDeviation) {
    // ARRANGE: High task-to-robot ratio
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14), Task(2, 1, 7), Task(3, 8, 14),
        Task(4, 7, 16), Task(5, 6, 13), Task(6, 9, 16)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 30.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 25.0);
}

TEST_F(OptimalComparisonTest, ClusteredTasks_4Tasks2Robots_GoodPerformance) {
    // ARRANGE: Tasks clustered in nearby nodes
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14),
        Task(2, 1, 7),
        Task(3, 0, 8),
        Task(4, 1, 5)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: Clustered tasks should be easier for heuristics
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 35.0) << "Greedy should be within 35% for clustered tasks";
    ASSERT_LE(result.hillClimbing_deviation_percent, 15.0) << "HC should be within 15% for clustered tasks";
}

TEST_F(OptimalComparisonTest, DistributedTasks_5Tasks2Robots_ModerateDeviation) {
    // ARRANGE: Tasks spread across graph
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 6, 14), Task(2, 3, 7), Task(3, 6, 8),
        Task(4, 2, 5), Task(5, 5, 6)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 30.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 25.0);
}

TEST_F(OptimalComparisonTest, SingleRobotManyTasks_5Tasks1Robot_OptimalMatches) {
    // ARRANGE: 1 robot, multiple tasks - order matters less
    const int numRobots = 1;
    std::vector<Task> tasks = {
        Task(1, 6, 14), Task(2, 7, 13), Task(3, 4, 8),
        Task(4, 1, 5), Task(5, 3, 6)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: With 1 robot, TSP ordering is key
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 35.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 30.0);
}

TEST_F(OptimalComparisonTest, ManyRobotsFewTasks_2Tasks4Robots_NearOptimal) {
    // ARRANGE: More robots than tasks
    const int numRobots = 4;
    std::vector<Task> tasks = {
        Task(1, 6, 14),
        Task(2, 5, 8)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: With abundant robots, should be nearly optimal
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 10.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 5.0);
}

TEST_F(OptimalComparisonTest, VariedTaskDurations_4Tasks2Robots_CloseToOptimal) {
    // ARRANGE: Tasks with varied durations
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 0, 10),  // Long task
        Task(2, 1, 3),   // Short task
        Task(3, 2, 15),  // Very long task
        Task(4, 7, 16)    // Medium task
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 25.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 20.0);
}

// =============================================================================
// META TEST: Verify test infrastructure
// =============================================================================

TEST_F(OptimalComparisonTest, TestInfrastructure_AllAlgorithmsRun) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {Task(1, 6, 14), Task(2, 8, 12)};

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: All algorithms should produce valid results
    ASSERT_GT(result.bruteForce_makespan, 0.0) << "BruteForce should complete";
    ASSERT_GT(result.greedy_makespan, 0.0) << "Greedy should complete";
    ASSERT_GT(result.hillClimbing_makespan, 0.0) << "HillClimbing should complete";
    
    // Deviations should be calculated
    ASSERT_GE(result.greedy_deviation_percent, 0.0);
    ASSERT_GE(result.hillClimbing_deviation_percent, 0.0);
}

TEST_F(OptimalComparisonTest, LargeProblem_10Tasks_3Robots_QualityCheck) {
    // ARRANGE: Larger problem to stress test heuristics
    const int numRobots = 3;
    std::vector<Task> tasks;
    for (int i = 1; i <= 10; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 7) + 12));
    }

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT: Heuristics should still be reasonable on larger problems
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 40.0)
        << "Greedy should be within 40% for 10-task problems";
    ASSERT_LE(result.hillClimbing_deviation_percent, 35.0)
        << "HC should be within 35% for 10-task problems";
}

TEST_F(OptimalComparisonTest, BalancedLoad_6Tasks_3Robots_GoodQuality) {
    // ARRANGE: Perfect balance scenario
    const int numRobots = 3;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 6, 13), Task(3, 7, 14),
        Task(4, 8, 15), Task(5, 9, 16), Task(6, 10, 17)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 30.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 25.0);
}

TEST_F(OptimalComparisonTest, AsymmetricLoad_7Tasks_3Robots_ReasonableQuality) {
    // ARRANGE: Uneven task count
    const int numRobots = 3;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 6, 13), Task(3, 7, 14),
        Task(4, 8, 15), Task(5, 9, 16), Task(6, 10, 17),
        Task(7, 11, 18)
    };

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 35.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 30.0);
}

TEST_F(OptimalComparisonTest, EdgeCase_8Tasks_4Robots_HeuristicsReasonable) {
    // ARRANGE: Edge case with even distribution
    const int numRobots = 4;
    std::vector<Task> tasks;
    for (int i = 1; i <= 8; i++) {
        tasks.push_back(Task(i, (i % 6) + 5, (i % 6) + 12));
    }

    // ACT
    ComparisonResult result = runComparison(numRobots, tasks);

    // ASSERT
    ASSERT_GT(result.bruteForce_makespan, 0.0);
    ASSERT_LE(result.greedy_deviation_percent, 40.0);
    ASSERT_LE(result.hillClimbing_deviation_percent, 35.0);
}

