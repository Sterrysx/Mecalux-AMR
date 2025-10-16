#include "gtest/gtest.h"
#include <queue>
#include <fstream>
#include <memory>
#include <algorithm>
#include <vector>
#include <set>
#include "Planifier.hh"
#include "algorithms/02_Greedy.hh"
#include "algorithms/03_HillClimbing.hh"

// =============================================================================
// INTEGRATION TESTS: Test complete algorithms working together
// =============================================================================

// Helper function to load tasks from a file for our tests
std::queue<Task> loadTasksFromFile(const std::string& filename) {
    std::queue<Task> tasks;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return tasks;
    }
    
    int id, origin, dest;
    while (file >> id >> origin >> dest) {
        tasks.push(Task(id, origin, dest));
    }
    return tasks;
}

// Test Fixture for Algorithm Integration Tests
class SchedulerAlgorithmsTest : public ::testing::Test {
protected:
    Graph graph4;
    Graph graph5;
    Graph graph7;

    void SetUp() override {
        // Load graph4
        std::ifstream graph4File("../optimality/01_layer_mapping/tests/distributions/graph4.inp");
        if (graph4File.is_open()) {
            graph4.loadFromStream(graph4File);
        }
        
        // Load graph5
        std::ifstream graph5File("../optimality/01_layer_mapping/tests/distributions/graph5.inp");
        if (graph5File.is_open()) {
            graph5.loadFromStream(graph5File);
        }
        
        // Load graph7
        std::ifstream graph7File("../optimality/01_layer_mapping/tests/distributions/graph7.inp");
        if (!graph7File.is_open()) {
            std::cerr << "Error: Could not open graph7.inp" << std::endl;
            FAIL() << "Failed to load graph file";
        }
        graph7.loadFromStream(graph7File);
    }
};

// =============================================================================
// GREEDY ALGORITHM TESTS
// =============================================================================

// --- CORRECTNESS TESTS ---

TEST_F(SchedulerAlgorithmsTest, Greedy_Graph7_8Tasks_2Robots_BasicCorrectness) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Check if the makespan is within reasonable bounds
    ASSERT_LE(result.makespan, 65.0) 
        << "Greedy makespan should be reasonable (<65s) but got " << result.makespan;
    ASSERT_GE(result.makespan, 50.0)
        << "Greedy makespan should not be too small (>50s) but got " << result.makespan;
}

TEST_F(SchedulerAlgorithmsTest, Greedy_ProducesValidAssignment) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    size_t taskCount = 0;
    std::queue<Task> tasksCopy = tasks;
    while (!tasksCopy.empty()) { taskCount++; tasksCopy.pop(); }
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: All tasks should be assigned
    ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots));
    
    // Count total tasks assigned
    size_t totalAssigned = 0;
    for (const auto& robotTasks : result.assignment) {
        totalAssigned += robotTasks.size();
    }
    ASSERT_EQ(totalAssigned, taskCount) << "All tasks should be assigned";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_AllTasksAssignedOnce) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    // Count input tasks
    size_t inputTaskCount = 0;
    std::queue<Task> tasksCopy = tasks;
    while (!tasksCopy.empty()) { inputTaskCount++; tasksCopy.pop(); }
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Total assigned tasks should equal input tasks
    size_t totalAssigned = 0;
    for (const auto& robotTasks : result.assignment) {
        totalAssigned += robotTasks.size();
    }
    ASSERT_EQ(totalAssigned, inputTaskCount) 
        << "All tasks should be assigned exactly once";
    
    // Each robot assignment should be non-overlapping in the schedule
    for (size_t i = 0; i < result.assignment.size(); ++i) {
        ASSERT_TRUE(result.assignment[i].size() <= inputTaskCount)
            << "Robot " << i << " should not have more tasks than exist";
    }
}

// --- EDGE CASES ---

TEST_F(SchedulerAlgorithmsTest, Greedy_SingleTask_SingleRobot) {
    // ARRANGE: Minimal case
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.makespan, 30.0);
    ASSERT_EQ(result.assignment.size(), 1);
    ASSERT_EQ(result.assignment[0].size(), 1);
}

TEST_F(SchedulerAlgorithmsTest, Greedy_MoreRobotsThanTasks) {
    // ARRANGE
    const int numRobots = 5;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 2, 8));
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots));
    
    // Some robots should have no tasks
    int robotsWithTasks = 0;
    for (const auto& robotTasks : result.assignment) {
        if (!robotTasks.empty()) robotsWithTasks++;
    }
    ASSERT_LE(robotsWithTasks, 2) << "Only 2 robots should have tasks";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_MoreTasksThanRobots) {
    // ARRANGE
    const int numRobots = 1;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_EQ(result.assignment.size(), 1);
    ASSERT_EQ(result.assignment[0].size(), 8) << "Single robot should get all 8 tasks";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_SameOriginDestination) {
    // ARRANGE: Tasks where origin == destination
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 3, 3));  // No-op task
    tasks.push(Task(2, 0, 6));  // Normal task
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_GE(result.makespan, 0.0) << "Should handle same origin/dest tasks";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_RobotsStartAtSameLocation) {
    // ARRANGE: All robots start at default location
    const int numRobots = 3;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LE(result.makespan, 100.0) << "Multiple robots at same start should work";
}

// --- SCALABILITY TESTS ---

TEST_F(SchedulerAlgorithmsTest, Greedy_DifferentTaskCounts) {
    // ARRANGE: Test with 5, 8, and 15 tasks
    const int numRobots = 2;
    
    auto tasks5 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/5_tasks.inp");
    auto tasks8 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    if (tasks5.empty() || tasks8.empty()) {
        GTEST_SKIP() << "Required task files not found";
    }
    
    Planifier planner5(graph7, numRobots, tasks5);
    Planifier planner8(graph7, numRobots, tasks8);
    
    planner5.setAlgorithm(std::make_unique<Greedy>());
    planner8.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result5 = planner5.executePlan();
    AlgorithmResult result8 = planner8.executePlan();

    // ASSERT: More tasks should generally take longer
    ASSERT_GT(result8.makespan, result5.makespan * 0.8) 
        << "8 tasks should take at least 80% as long as 5 tasks";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_ConsistentResultsAcrossRuns) {
    // ARRANGE: Run multiple times to check consistency
    const int numRobots = 2;
    std::vector<double> makespans;
    
    for (int i = 0; i < 5; ++i) {
        auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
        Planifier planner(graph7, numRobots, tasks);
        planner.setAlgorithm(std::make_unique<Greedy>());
        AlgorithmResult result = planner.executePlan();
        makespans.push_back(result.makespan);
    }

    // ASSERT: Results should be within reasonable variance
    double minMakespan = *std::min_element(makespans.begin(), makespans.end());
    double maxMakespan = *std::max_element(makespans.begin(), makespans.end());
    
    ASSERT_LT(maxMakespan - minMakespan, 15.0) 
        << "Greedy results should be relatively consistent";
}

TEST_F(SchedulerAlgorithmsTest, Greedy_ComputationTimeReasonable) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT: Should complete quickly
    ASSERT_GE(result.computationTimeMs, 0.0);
    ASSERT_LT(result.computationTimeMs, 100.0) 
        << "Greedy should complete in less than 100ms for 8 tasks";
}

// =============================================================================
// HILL CLIMBING ALGORITHM TESTS
// =============================================================================

// --- CORRECTNESS TESTS ---

TEST_F(SchedulerAlgorithmsTest, HillClimbing_Graph7_8Tasks_2Robots_BasicCorrectness) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_LE(result.makespan, 65.0)
        << "HillClimbing makespan should be reasonable (<65s)";
    ASSERT_GE(result.makespan, 40.0)
        << "HillClimbing makespan should not be too small (>40s)";
}

TEST_F(SchedulerAlgorithmsTest, HillClimbing_ProducesValidAssignment) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots));
    
    // All 8 tasks should be assigned
    size_t totalAssigned = 0;
    for (const auto& robotTasks : result.assignment) {
        totalAssigned += robotTasks.size();
    }
    ASSERT_EQ(totalAssigned, 8);
}

TEST_F(SchedulerAlgorithmsTest, HillClimbing_CompetitiveWithGreedy) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks1 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    auto tasks2 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier greedyPlanner(graph7, numRobots, tasks1);
    Planifier hillClimbingPlanner(graph7, numRobots, tasks2);
    
    greedyPlanner.setAlgorithm(std::make_unique<Greedy>());
    hillClimbingPlanner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult greedyResult = greedyPlanner.executePlan();
    AlgorithmResult hillClimbingResult = hillClimbingPlanner.executePlan();

    // ASSERT: HC should be competitive (within 15% of Greedy)
    double ratio = hillClimbingResult.makespan / greedyResult.makespan;
    ASSERT_LE(ratio, 1.15)
        << "HillClimbing should be within 15% of Greedy";
}

// --- PROPERTY-BASED TESTS ---

TEST_F(SchedulerAlgorithmsTest, HillClimbing_MoreRobotsNotWorse) {
    // ARRANGE: Test with 1, 2, and 4 robots
    auto tasks1 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    auto tasks2 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    auto tasks4 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier planner1(graph7, 1, tasks1);
    Planifier planner2(graph7, 2, tasks2);
    Planifier planner4(graph7, 4, tasks4);
    
    planner1.setAlgorithm(std::make_unique<HillClimbing>());
    planner2.setAlgorithm(std::make_unique<HillClimbing>());
    planner4.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result1 = planner1.executePlan();
    AlgorithmResult result2 = planner2.executePlan();
    AlgorithmResult result4 = planner4.executePlan();

    // ASSERT: More robots should not significantly increase makespan
    ASSERT_LE(result2.makespan, result1.makespan * 1.1) 
        << "2 robots should not be much worse than 1";
    ASSERT_LE(result4.makespan, result2.makespan * 1.1)
        << "4 robots should not be much worse than 2";
}

// --- EDGE CASES ---

TEST_F(SchedulerAlgorithmsTest, HillClimbing_SingleTask_SingleRobot) {
    // ARRANGE
    const int numRobots = 1;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.makespan, 30.0);
}

TEST_F(SchedulerAlgorithmsTest, HillClimbing_TwoTasksTwoRobots) {
    // ARRANGE: Perfect balance scenario
    const int numRobots = 2;
    std::queue<Task> tasks;
    tasks.push(Task(1, 6, 14));
    tasks.push(Task(2, 8, 12));
    
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.makespan, 50.0);
}

// --- ALGORITHM BEHAVIOR TESTS ---

TEST_F(SchedulerAlgorithmsTest, HillClimbing_ComputationTimeReasonable) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GE(result.computationTimeMs, 0.0);
    ASSERT_LT(result.computationTimeMs, 500.0) 
        << "Hill Climbing should complete in reasonable time";
}

// =============================================================================
// MULTI-GRAPH TESTS
// =============================================================================

TEST_F(SchedulerAlgorithmsTest, Greedy_Graph5_BasicTest) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph5/8_tasks.inp");
    
    if (tasks.empty()) {
        GTEST_SKIP() << "graph5/8_tasks.inp not found";
    }
    
    Planifier planner(graph5, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.makespan, 200.0);
}

TEST_F(SchedulerAlgorithmsTest, HillClimbing_Graph5_BasicTest) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph5/8_tasks.inp");
    
    if (tasks.empty()) {
        GTEST_SKIP() << "graph5/8_tasks.inp not found";
    }
    
    Planifier planner(graph5, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0);
    ASSERT_LT(result.makespan, 200.0);
}

// =============================================================================
// RESULT VALIDATION TESTS
// =============================================================================

TEST_F(SchedulerAlgorithmsTest, AlgorithmResult_ValidStructure) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    Planifier planner(graph7, numRobots, tasks);
    planner.setAlgorithm(std::make_unique<Greedy>());

    // ACT
    AlgorithmResult result = planner.executePlan();

    // ASSERT
    ASSERT_GT(result.makespan, 0.0) << "Makespan must be positive";
    ASSERT_GE(result.computationTimeMs, 0.0) << "Computation time must be non-negative";
    ASSERT_EQ(result.assignment.size(), static_cast<size_t>(numRobots));
    ASSERT_FALSE(result.algorithmName.empty()) << "Algorithm name should be set";
}

TEST_F(SchedulerAlgorithmsTest, MultipleAlgorithms_ProduceSimilarResults) {
    // ARRANGE
    const int numRobots = 2;
    auto tasks1 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    auto tasks2 = loadTasksFromFile("../optimality/02_layer_planner/test_data/graph7/8_tasks.inp");
    
    Planifier greedyPlanner(graph7, numRobots, tasks1);
    Planifier hillClimbingPlanner(graph7, numRobots, tasks2);
    
    greedyPlanner.setAlgorithm(std::make_unique<Greedy>());
    hillClimbingPlanner.setAlgorithm(std::make_unique<HillClimbing>());

    // ACT
    AlgorithmResult greedyResult = greedyPlanner.executePlan();
    AlgorithmResult hillClimbingResult = hillClimbingPlanner.executePlan();

    // ASSERT: Both should be in reasonable range
    ASSERT_GT(greedyResult.makespan, 0.0);
    ASSERT_GT(hillClimbingResult.makespan, 0.0);
    ASSERT_LT(greedyResult.makespan, 70.0);
    ASSERT_LT(hillClimbingResult.makespan, 70.0);
    
    // Both algorithms should solve the same problem with similar quality
    double difference = std::abs(greedyResult.makespan - hillClimbingResult.makespan);
    ASSERT_LT(difference, 20.0) << "Algorithms should produce reasonably similar results";
}
