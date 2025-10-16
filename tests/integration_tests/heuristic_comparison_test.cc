#include "gtest/gtest.h"
#include <queue>
#include <memory>
#include <fstream>
#include "../../optimality/02_layer_planner/include/Planifier.hh"
#include "../../optimality/02_layer_planner/include/algorithms/02_Greedy.hh"
#include "../../optimality/02_layer_planner/include/algorithms/03_HillClimbing.hh"
#include "../../optimality/01_layer_mapping/include/Graph.hh"

// =============================================================================
// INTEGRATION TESTS: HillClimbing vs Greedy Performance Comparison
// =============================================================================
// Purpose: Verify that HillClimbing always produces solutions >= Greedy
//          (HC should improve or maintain greedy baseline)
// =============================================================================

class HeuristicComparisonTest : public ::testing::Test {
protected:
    Graph testGraph;

    void SetUp() override {
        std::ifstream graphFile("../optimality/01_layer_mapping/tests/distributions/graph7.inp");
        if (graphFile.is_open()) {
            testGraph.loadFromStream(graphFile);
        }
    }

    struct HeuristicResult {
        double greedy_makespan;
        double hillClimbing_makespan;
        double improvement_percent;  // (greedy - HC) / greedy * 100 (positive = improvement)
        bool hillClimbing_improved;
    };

    HeuristicResult runHeuristicComparison(int numRobots, std::vector<Task> taskList) {
        HeuristicResult result;

        // Run Greedy
        std::queue<Task> tasks1;
        for (const auto& t : taskList) tasks1.push(t);
        Planifier planner1(testGraph, numRobots, tasks1);
        planner1.setAlgorithm(std::make_unique<Greedy>());
        AlgorithmResult greedy_result = planner1.executePlan();
        result.greedy_makespan = greedy_result.makespan;

        // Run HillClimbing
        std::queue<Task> tasks2;
        for (const auto& t : taskList) tasks2.push(t);
        Planifier planner2(testGraph, numRobots, tasks2);
        planner2.setAlgorithm(std::make_unique<HillClimbing>());
        AlgorithmResult hc_result = planner2.executePlan();
        result.hillClimbing_makespan = hc_result.makespan;

        // Calculate improvement (positive = HC is better)
        if (result.greedy_makespan > 0) {
            result.improvement_percent = 
                ((result.greedy_makespan - result.hillClimbing_makespan) / result.greedy_makespan) * 100.0;
        } else {
            result.improvement_percent = 0.0;
        }

        result.hillClimbing_improved = (result.hillClimbing_makespan <= result.greedy_makespan);

        return result;
    }
};

// --- TEST CASES ---

TEST_F(HeuristicComparisonTest, SmallProblem_2Tasks2Robots_HCImproves) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {Task(1, 6, 14), Task(2, 8, 12)};

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should be <= Greedy (lower is better)
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan)
        << "HC makespan: " << result.hillClimbing_makespan 
        << ", Greedy makespan: " << result.greedy_makespan;
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, MediumProblem_4Tasks2Robots_HCNotWorse) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 7, 15), Task(4, 8, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_GE(result.improvement_percent, 0.0) << "Should show improvement or equal";
}

TEST_F(HeuristicComparisonTest, LargeProblem_8Tasks2Robots_HCShines) {
    // ARRANGE: Larger problems where HC local search should help
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 7, 15), Task(4, 8, 12),
        Task(5, 9, 13), Task(6, 10, 14), Task(7, 11, 15), Task(8, 5, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should improve on larger problems
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, HighLoad_6Tasks2Robots_HCNotWorse) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 7, 15),
        Task(4, 8, 12), Task(5, 9, 13), Task(6, 10, 14)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, UnbalancedLoad_3Tasks3Robots_HCImproves) {
    // ARRANGE
    const int numRobots = 3;
    std::vector<Task> tasks = {Task(1, 5, 13), Task(2, 7, 15), Task(3, 9, 14)};

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, ClusteredTasks_4Tasks2Robots_HCNotWorse) {
    // ARRANGE: Clustered tasks
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 5, 15), Task(4, 6, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, DistributedTasks_5Tasks2Robots_HCImproves) {
    // ARRANGE: Tasks spread across graph
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 8, 14), Task(3, 11, 15),
        Task(4, 7, 12), Task(5, 10, 13)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, SingleRobot_5Tasks_HCNotWorse) {
    // ARRANGE: Single robot
    const int numRobots = 1;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 7, 14), Task(3, 9, 15),
        Task(4, 6, 12), Task(5, 8, 13)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Even with 1 robot, HC should not be worse
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, ManyRobots_2Tasks4Robots_HCNotWorse) {
    // ARRANGE: More robots than tasks
    const int numRobots = 4;
    std::vector<Task> tasks = {Task(1, 5, 13), Task(2, 10, 15)};

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, VariedDurations_4Tasks2Robots_HCImproves) {
    // ARRANGE: Tasks with varied durations
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 17), Task(2, 6, 12), Task(3, 7, 18), Task(4, 8, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, AsymmetricLoad_7Tasks3Robots_HCNotWorse) {
    // ARRANGE
    const int numRobots = 3;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 7, 15), Task(4, 8, 12),
        Task(5, 9, 13), Task(6, 10, 14), Task(7, 11, 15)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, ShortTasks_5Tasks2Robots_HCNotWorse) {
    // ARRANGE: All short duration tasks (closer dropoff nodes)
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 6, 12), Task(3, 7, 12),
        Task(4, 8, 12), Task(5, 9, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, LongTasks_4Tasks2Robots_HCImproves) {
    // ARRANGE: All long duration tasks (farther dropoff nodes)
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 18), Task(2, 6, 18), Task(3, 7, 18), Task(4, 8, 17)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, EdgeCase_SingleTask_HCEqualsGreedy) {
    // ARRANGE: Single task - no room for improvement
    const int numRobots = 1;
    std::vector<Task> tasks = {Task(1, 6, 14)};

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Should be equal (no swaps possible)
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_EQ(result.improvement_percent, 0.0);
}

TEST_F(HeuristicComparisonTest, Scenario_SequentialNodes_6Tasks2Robots_HCNotWorse) {
    // ARRANGE: Tasks on sequential nodes
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 6, 13), Task(3, 7, 14),
        Task(4, 8, 12), Task(5, 9, 13), Task(6, 10, 14)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, Scenario_AlternatingNodes_6Tasks2Robots_HCImproves) {
    // ARRANGE: Tasks on alternating pickup nodes
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 7, 13), Task(3, 9, 14),
        Task(4, 6, 12), Task(5, 8, 13), Task(6, 10, 14)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should improve by reordering
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, Scenario_MixedLoad_10Tasks3Robots_HCNotWorse) {
    // ARRANGE: Large mixed scenario
    const int numRobots = 3;
    std::vector<Task> tasks = {
        Task(1, 5, 13), Task(2, 6, 14), Task(3, 7, 15), Task(4, 8, 12), Task(5, 9, 13),
        Task(6, 10, 14), Task(7, 11, 15), Task(8, 5, 12), Task(9, 6, 13), Task(10, 7, 14)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should handle large problems well
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, Scenario_StarPattern_5Tasks_HCImproves) {
    // ARRANGE: Central pickup node with tasks going to different dropoffs
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 9, 12),  // Center pickup node 9
        Task(2, 5, 13),  // Different pickup
        Task(3, 7, 14),  // Different pickup
        Task(4, 11, 15), // Different pickup
        Task(5, 8, 12)   // Different pickup
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_TRUE(result.hillClimbing_improved);
}

TEST_F(HeuristicComparisonTest, Scenario_ExtremeDurations_4Tasks2Robots_HCNotWorse) {
    // ARRANGE: One very far dropoff, rest closer
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 30),  // Very far dropoff (if exists) - use node 18 instead
        Task(2, 6, 12),  // Close dropoff
        Task(3, 7, 12),  // Close dropoff
        Task(4, 8, 12)   // Close dropoff
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should balance load better
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, Scenario_UniformTasks_8Tasks2Robots_HCNotWorse) {
    // ARRANGE: All tasks to same dropoff node
    const int numRobots = 2;
    std::vector<Task> tasks = {
        Task(1, 5, 12), Task(2, 6, 12), Task(3, 7, 12), Task(4, 8, 12),
        Task(5, 9, 12), Task(6, 10, 12), Task(7, 11, 12), Task(8, 5, 12)
    };

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

// =============================================================================
// META TEST: Verify test infrastructure
// =============================================================================

TEST_F(HeuristicComparisonTest, TestInfrastructure_BothAlgorithmsRun) {
    // ARRANGE
    const int numRobots = 2;
    std::vector<Task> tasks = {Task(1, 6, 14), Task(2, 8, 12)};

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Both algorithms should complete
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    
    // Improvement calculation should work
    ASSERT_GE(result.improvement_percent, -100.0);  // Can't be worse than 100% degradation
    ASSERT_LE(result.improvement_percent, 100.0);   // Can't improve more than 100%
}

TEST_F(HeuristicComparisonTest, LargeStress_10Tasks_3Robots_HCImproves) {
    // ARRANGE: Stress test with many tasks
    const int numRobots = 3;
    std::vector<Task> tasks;
    for (int i = 1; i <= 10; i++) {
        tasks.push_back(Task(i, (i % 8) + 4, (i % 8) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should match or improve Greedy on larger problems
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_GE(result.improvement_percent, 0.0);
}

TEST_F(HeuristicComparisonTest, VeryLargeStress_15Tasks_4Robots_HCImproves) {
    // ARRANGE: Very large stress test
    const int numRobots = 4;
    std::vector<Task> tasks;
    for (int i = 1; i <= 15; i++) {
        tasks.push_back(Task(i, (i % 9) + 5, (i % 9) + 14));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should always be at least as good as Greedy
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_GE(result.improvement_percent, 0.0);
}

TEST_F(HeuristicComparisonTest, Clustered_12Tasks_3Robots_HCFindsLocal) {
    // ARRANGE: Clustered tasks that Greedy might struggle with
    const int numRobots = 3;
    std::vector<Task> tasks;
    // Cluster 1: low nodes
    for (int i = 1; i <= 4; i++) {
        tasks.push_back(Task(i, 5, 12));
    }
    // Cluster 2: mid nodes
    for (int i = 5; i <= 8; i++) {
        tasks.push_back(Task(i, 10, 17));
    }
    // Cluster 3: high nodes
    for (int i = 9; i <= 12; i++) {
        tasks.push_back(Task(i, 15, 22));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should improve on clustered patterns
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
    ASSERT_GE(result.improvement_percent, 0.0);
}

// =============================================================================
// MASSIVE STRESS TESTS: Push heuristics to their limits (50-300 tasks)
// =============================================================================

TEST_F(HeuristicComparisonTest, MassiveStress_50Tasks_5Robots_HCScales) {
    // ARRANGE: 50 tasks - testing scalability
    const int numRobots = 5;
    std::vector<Task> tasks;
    for (int i = 1; i <= 50; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 7) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Both should complete, HC should not be worse
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, MassiveStress_100Tasks_10Robots_FastExecution) {
    // ARRANGE: 100 tasks - real-world warehouse scale
    const int numRobots = 10;
    std::vector<Task> tasks;
    for (int i = 1; i <= 100; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 10) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Should complete quickly and HC should optimize well
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, MassiveStress_150Tasks_15Robots_LoadBalancing) {
    // ARRANGE: 150 tasks - heavy load scenario
    const int numRobots = 15;
    std::vector<Task> tasks;
    for (int i = 1; i <= 150; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 11) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should find better load balancing
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, MassiveStress_200Tasks_20Robots_Industrial) {
    // ARRANGE: 200 tasks - industrial warehouse scale
    const int numRobots = 20;
    std::vector<Task> tasks;
    for (int i = 1; i <= 200; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 11) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Both algorithms should handle large scale
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, MassiveStress_250Tasks_25Robots_PeakLoad) {
    // ARRANGE: 250 tasks - peak load testing
    const int numRobots = 25;
    std::vector<Task> tasks;
    for (int i = 1; i <= 250; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 11) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Validate extreme scale performance
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, MassiveStress_300Tasks_30Robots_MaxCapacity) {
    // ARRANGE: 300 tasks - maximum capacity test
    const int numRobots = 30;
    std::vector<Task> tasks;
    for (int i = 1; i <= 300; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 11) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: Ultimate stress test - both should complete
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, AsymmetricStress_100Tasks_5Robots_HighCompetition) {
    // ARRANGE: Many tasks, few robots - competition for resources
    const int numRobots = 5;
    std::vector<Task> tasks;
    for (int i = 1; i <= 100; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, (i % 10) + 12));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should optimize better under high competition
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, ClusteredMassive_80Tasks_8Robots_MultiCluster) {
    // ARRANGE: 80 tasks in 4 clusters - spatial optimization challenge
    const int numRobots = 8;
    std::vector<Task> tasks;
    // Cluster pattern: alternate between pickup zones
    for (int i = 1; i <= 80; i++) {
        int cluster = (i - 1) / 20;  // 4 clusters of 20 tasks
        int pickup = 5 + (cluster % 3) * 2;  // Nodes 5, 7, 9, or 5
        int dropoff = 12 + (cluster % 4) * 2;  // Nodes 12, 14, 16, 18
        tasks.push_back(Task(i, pickup, dropoff));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should excel at cluster optimization
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, UniformMassive_120Tasks_12Robots_SameDestination) {
    // ARRANGE: 120 tasks to same dropoff - worst case for load balancing
    const int numRobots = 12;
    std::vector<Task> tasks;
    for (int i = 1; i <= 120; i++) {
        tasks.push_back(Task(i, (i % 7) + 5, 14));  // All to node 14
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should handle uniform distribution
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}

TEST_F(HeuristicComparisonTest, DistributedMassive_90Tasks_9Robots_MaxSpread) {
    // ARRANGE: 90 tasks maximally spread across graph
    const int numRobots = 9;
    std::vector<Task> tasks;
    for (int i = 1; i <= 90; i++) {
        // Maximize pickup-dropoff distance variation
        int pickup = 5 + (i % 7);
        int dropoff = 12 + ((i * 3) % 11);
        tasks.push_back(Task(i, pickup, dropoff));
    }

    // ACT
    HeuristicResult result = runHeuristicComparison(numRobots, tasks);

    // ASSERT: HC should optimize distributed patterns
    ASSERT_GT(result.greedy_makespan, 0.0);
    ASSERT_GT(result.hillClimbing_makespan, 0.0);
    ASSERT_LE(result.hillClimbing_makespan, result.greedy_makespan);
}
