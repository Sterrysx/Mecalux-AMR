#include "gtest/gtest.h"
#include "../../optimality/02_layer_planner/include/algorithms/utils/SchedulerUtils.hh"
#include "../../optimality/02_layer_planner/include/algorithms/utils/TSPSolver.hh"
#include "../../optimality/01_layer_mapping/include/Graph.hh"
#include "../../optimality/02_layer_planner/include/Robot.hh"
#include "../../optimality/02_layer_planner/include/Task.hh"
#include <cmath>

// =============================================================================
// UNIT TESTS: Test the smallest components in isolation
// =============================================================================

// Test fixture for SchedulerUtils functions
class SchedulerUtilsTest : public ::testing::Test {
protected:
    Graph testGraph;

    void SetUp() override {
        // Create a simple, predictable graph for testing
        // Node layout:
        // (0,0) -> (3,4) -> (10,10) [Charging]
        testGraph.addNode(0, Graph::NodeType::Waypoint, 0.0, 0.0);
        testGraph.addNode(1, Graph::NodeType::Waypoint, 3.0, 4.0);
        testGraph.addNode(2, Graph::NodeType::Charging, 10.0, 10.0);
        testGraph.addNode(3, Graph::NodeType::Waypoint, 6.0, 8.0);
        
        // Add edges for connectivity
        testGraph.addEdge(0, 1, 5.0, 1.6);   // Distance 5.0
        testGraph.addEdge(1, 2, 10.0, 1.6);  // Distance ~10
        testGraph.addEdge(1, 3, 5.0, 1.6);
        testGraph.addEdge(3, 2, 5.0, 1.6);
    }
    
    // Helper to create a test robot
    Robot createTestRobot() {
        return Robot(0, {0.0, 0.0}, 100.0, -1, 1.6, 2.0);
    }
};

// --- TEST CASES for SchedulerUtils::calculateDistance ---

TEST_F(SchedulerUtilsTest, CalculateDistance_Origin) {
    // ARRANGE
    std::pair<double, double> p1 = {0.0, 0.0};
    std::pair<double, double> p2 = {0.0, 0.0};

    // ACT
    double distance = SchedulerUtils::calculateDistance(p1, p2);

    // ASSERT
    ASSERT_DOUBLE_EQ(distance, 0.0) << "Distance from origin to itself should be 0";
}

TEST_F(SchedulerUtilsTest, CalculateDistance_PythagoreanTriple) {
    // ARRANGE: A known 3-4-5 right triangle
    std::pair<double, double> p1 = {0.0, 0.0};
    std::pair<double, double> p2 = {3.0, 4.0};

    // ACT
    double distance = SchedulerUtils::calculateDistance(p1, p2);

    // ASSERT
    ASSERT_DOUBLE_EQ(distance, 5.0) << "3-4-5 triangle should give distance 5.0";
}

TEST_F(SchedulerUtilsTest, CalculateDistance_NegativeCoordinates) {
    // ARRANGE
    std::pair<double, double> p1 = {-3.0, -4.0};
    std::pair<double, double> p2 = {0.0, 0.0};

    // ACT
    double distance = SchedulerUtils::calculateDistance(p1, p2);

    // ASSERT
    ASSERT_DOUBLE_EQ(distance, 5.0) << "Distance should work with negative coordinates";
}

TEST_F(SchedulerUtilsTest, CalculateDistance_Symmetry) {
    // ARRANGE
    std::pair<double, double> p1 = {1.0, 2.0};
    std::pair<double, double> p2 = {4.0, 6.0};

    // ACT
    double distance1 = SchedulerUtils::calculateDistance(p1, p2);
    double distance2 = SchedulerUtils::calculateDistance(p2, p1);

    // ASSERT
    ASSERT_DOUBLE_EQ(distance1, distance2) << "Distance should be symmetric";
}

TEST_F(SchedulerUtilsTest, CalculateDistance_LargeValues) {
    // ARRANGE
    std::pair<double, double> p1 = {0.0, 0.0};
    std::pair<double, double> p2 = {1000.0, 1000.0};

    // ACT
    double distance = SchedulerUtils::calculateDistance(p1, p2);

    // ASSERT
    ASSERT_NEAR(distance, 1414.21356, 0.001) << "Should handle large coordinate values";
}

// --- TEST CASES for SchedulerUtils::getChargingNodeId ---

TEST_F(SchedulerUtilsTest, GetChargingNodeId_FindsCorrectNode) {
    // ARRANGE: The graph has a charging node with ID 2
    
    // ACT
    int chargingNodeId = SchedulerUtils::getChargingNodeId(testGraph);
    
    // ASSERT
    ASSERT_EQ(chargingNodeId, 2) << "Should find the charging station at node 2";
}

TEST_F(SchedulerUtilsTest, GetChargingNodeId_NoChargingStation) {
    // ARRANGE: Create a graph with no charging station
    Graph emptyGraph;
    emptyGraph.addNode(0, Graph::NodeType::Waypoint, 0.0, 0.0);
    emptyGraph.addNode(1, Graph::NodeType::Waypoint, 1.0, 1.0);
    
    // ACT
    int chargingNodeId = SchedulerUtils::getChargingNodeId(emptyGraph);
    
    // ASSERT
    ASSERT_EQ(chargingNodeId, -1) << "Should return -1 when no charging station exists";
}

TEST_F(SchedulerUtilsTest, GetChargingNodeId_MultipleStations) {
    // ARRANGE: Add another charging station
    testGraph.addNode(10, Graph::NodeType::Charging, 20.0, 20.0);
    
    // ACT
    int chargingNodeId = SchedulerUtils::getChargingNodeId(testGraph);
    
    // ASSERT: Should return the first one found (implementation dependent)
    ASSERT_TRUE(chargingNodeId == 2 || chargingNodeId == 10) 
        << "Should return a valid charging station ID";
}

// --- TEST CASES for SchedulerUtils::calculateTaskBatteryConsumption ---

TEST_F(SchedulerUtilsTest, CalculateTaskBatteryConsumption_ZeroDistance) {
    // ARRANGE: Task at current position
    std::pair<double, double> currentPos = {0.0, 0.0};
    const Graph::Node* originNode = testGraph.getNode(0);
    const Graph::Node* destNode = testGraph.getNode(0);
    double currentBattery = 100.0;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    auto info = SchedulerUtils::calculateTaskBatteryConsumption(
        currentPos, originNode, destNode, currentBattery, config
    );

    // ASSERT
    ASSERT_DOUBLE_EQ(info.distanceToOrigin, 0.0);
    ASSERT_DOUBLE_EQ(info.timeToOrigin, 0.0);
    ASSERT_DOUBLE_EQ(info.batteryToOrigin, 0.0);
    ASSERT_DOUBLE_EQ(info.distanceForTask, 0.0);
    ASSERT_DOUBLE_EQ(info.timeForTask, 0.0);
    ASSERT_DOUBLE_EQ(info.batteryForTask, 0.0);
    ASSERT_DOUBLE_EQ(info.totalBatteryNeeded, 0.0);
    ASSERT_DOUBLE_EQ(info.batteryAfterTask, 100.0);
}

TEST_F(SchedulerUtilsTest, CalculateTaskBatteryConsumption_KnownDistance) {
    // ARRANGE: Robot at origin, task from node 0 to node 1 (distance 5.0)
    std::pair<double, double> currentPos = {0.0, 0.0};
    const Graph::Node* originNode = testGraph.getNode(0);
    const Graph::Node* destNode = testGraph.getNode(1);  // At (3, 4), distance 5
    double currentBattery = 100.0;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    auto info = SchedulerUtils::calculateTaskBatteryConsumption(
        currentPos, originNode, destNode, currentBattery, config
    );

    // ASSERT
    ASSERT_DOUBLE_EQ(info.distanceToOrigin, 0.0) << "Already at origin";
    ASSERT_DOUBLE_EQ(info.distanceForTask, 5.0) << "Task distance should be 5.0";
    ASSERT_NEAR(info.timeForTask, 3.125, 0.001) << "Time = 5.0 / 1.6 = 3.125s";
    ASSERT_GT(info.batteryForTask, 0.0) << "Should consume some battery";
    ASSERT_LT(info.batteryAfterTask, 100.0) << "Battery should decrease";
}

TEST_F(SchedulerUtilsTest, CalculateTaskBatteryConsumption_TravelToOrigin) {
    // ARRANGE: Robot at node 1, task from node 0 to node 1
    std::pair<double, double> currentPos = {3.0, 4.0};
    const Graph::Node* originNode = testGraph.getNode(0);  // At (0, 0)
    const Graph::Node* destNode = testGraph.getNode(1);    // At (3, 4)
    double currentBattery = 100.0;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    auto info = SchedulerUtils::calculateTaskBatteryConsumption(
        currentPos, originNode, destNode, currentBattery, config
    );

    // ASSERT
    ASSERT_DOUBLE_EQ(info.distanceToOrigin, 5.0) << "Must travel distance 5 to origin";
    ASSERT_GT(info.batteryToOrigin, 0.0) << "Should consume battery traveling to origin";
    ASSERT_DOUBLE_EQ(info.distanceForTask, 5.0) << "Task distance is 5.0";
    ASSERT_GT(info.totalBatteryNeeded, info.batteryToOrigin) 
        << "Total should include both travel and task";
}

// --- TEST CASES for SchedulerUtils::shouldCharge ---

TEST_F(SchedulerUtilsTest, ShouldCharge_BelowThreshold) {
    // ARRANGE
    double batteryLevel = 15.0;
    double threshold = 20.0;

    // ACT
    bool shouldCharge = SchedulerUtils::shouldCharge(batteryLevel, threshold);

    // ASSERT
    ASSERT_TRUE(shouldCharge) << "Should charge when below threshold";
}

TEST_F(SchedulerUtilsTest, ShouldCharge_AboveThreshold) {
    // ARRANGE
    double batteryLevel = 25.0;
    double threshold = 20.0;

    // ACT
    bool shouldCharge = SchedulerUtils::shouldCharge(batteryLevel, threshold);

    // ASSERT
    ASSERT_FALSE(shouldCharge) << "Should not charge when above threshold";
}

TEST_F(SchedulerUtilsTest, ShouldCharge_ExactlyAtThreshold) {
    // ARRANGE
    double batteryLevel = 20.0;
    double threshold = 20.0;

    // ACT
    bool shouldCharge = SchedulerUtils::shouldCharge(batteryLevel, threshold);

    // ASSERT
    ASSERT_FALSE(shouldCharge) << "Should not charge when exactly at threshold";
}

TEST_F(SchedulerUtilsTest, ShouldCharge_ZeroBattery) {
    // ARRANGE
    double batteryLevel = 0.0;
    double threshold = 20.0;

    // ACT
    bool shouldCharge = SchedulerUtils::shouldCharge(batteryLevel, threshold);

    // ASSERT
    ASSERT_TRUE(shouldCharge) << "Should definitely charge at 0% battery";
}

// --- TEST CASES for SchedulerUtils::performCharging ---

TEST_F(SchedulerUtilsTest, PerformCharging_UpdatesPosition) {
    // ARRANGE
    std::pair<double, double> position = {0.0, 0.0};
    double battery = 50.0;
    double time = 0.0;
    int chargingNodeId = 2;  // At (10, 10)
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    SchedulerUtils::performCharging(position, battery, time, chargingNodeId, testGraph, config);
    
    // ASSERT
    ASSERT_DOUBLE_EQ(position.first, 10.0) << "Should move to charging station X";
    ASSERT_DOUBLE_EQ(position.second, 10.0) << "Should move to charging station Y";
}

TEST_F(SchedulerUtilsTest, PerformCharging_ChargesTo100Percent) {
    // ARRANGE
    std::pair<double, double> position = {10.0, 10.0};  // Already at charging station
    double battery = 50.0;
    double time = 0.0;
    int chargingNodeId = 2;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    SchedulerUtils::performCharging(position, battery, time, chargingNodeId, testGraph, config);
    
    // ASSERT
    ASSERT_DOUBLE_EQ(battery, 100.0) << "Should charge to 100%";
}

TEST_F(SchedulerUtilsTest, PerformCharging_UpdatesTime) {
    // ARRANGE
    std::pair<double, double> position = {10.0, 10.0};  // Already at station
    double battery = 50.0;
    double initialTime = 100.0;
    int chargingNodeId = 2;
    double time = initialTime;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    SchedulerUtils::performCharging(position, battery, time, chargingNodeId, testGraph, config);
    
    // ASSERT
    ASSERT_GT(time, initialTime) << "Time should increase during charging";
    // With 50% battery and 2% recharge rate: (100-50)/2 = 25 seconds
    ASSERT_NEAR(time, initialTime + 25.0, 0.01);
}

TEST_F(SchedulerUtilsTest, PerformCharging_AlreadyFullBattery) {
    // ARRANGE
    std::pair<double, double> position = {10.0, 10.0};
    double battery = 100.0;
    double initialTime = 50.0;
    int chargingNodeId = 2;
    double time = initialTime;
    Robot testRobot = createTestRobot();
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ACT
    SchedulerUtils::performCharging(position, battery, time, chargingNodeId, testGraph, config);
    
    // ASSERT
    ASSERT_DOUBLE_EQ(battery, 100.0) << "Battery stays at 100%";
    ASSERT_DOUBLE_EQ(time, initialTime) << "No time should pass if already full";
}

// =============================================================================
// UNIT TESTS: TSPSolver
// =============================================================================

class TSPSolverTest : public ::testing::Test {
protected:
    Graph simpleGraph;
    
    void SetUp() override {
        // Create a simple graph with known distances
        simpleGraph.addNode(0, Graph::NodeType::Waypoint, 0.0, 0.0);
        simpleGraph.addNode(1, Graph::NodeType::Waypoint, 1.0, 0.0);
        simpleGraph.addNode(2, Graph::NodeType::Waypoint, 2.0, 0.0);
        simpleGraph.addNode(3, Graph::NodeType::Waypoint, 3.0, 0.0);
        
        // Add edges
        simpleGraph.addEdge(0, 1, 1.0, 1.0);
        simpleGraph.addEdge(1, 2, 1.0, 1.0);
        simpleGraph.addEdge(2, 3, 1.0, 1.0);
        simpleGraph.addEdge(0, 2, 2.0, 1.0);
        simpleGraph.addEdge(1, 3, 2.0, 1.0);
        simpleGraph.addEdge(0, 3, 3.0, 1.0);
    }
};

TEST_F(TSPSolverTest, FindOptimalSequenceTime_EmptyTasks) {
    // ARRANGE
    Robot robot(0, {0.0, 0.0}, 100.0, -1, 1.0, 2.0);
    std::vector<Task> tasks;

    // ACT
    double time = TSPSolver::findOptimalSequenceTime(robot, tasks, simpleGraph);

    // ASSERT
    ASSERT_DOUBLE_EQ(time, 0.0) << "No tasks should take 0 time";
}

TEST_F(TSPSolverTest, FindOptimalSequenceTime_SingleTask) {
    // ARRANGE
    Robot robot(0, {0.0, 0.0}, 100.0, -1, 1.0, 2.0);
    std::vector<Task> tasks = {Task(1, 0, 1)};

    // ACT
    double time = TSPSolver::findOptimalSequenceTime(robot, tasks, simpleGraph);

    // ASSERT
    ASSERT_GT(time, 0.0) << "Single task should take some time";
    ASSERT_LT(time, 100.0) << "Time should be reasonable";
}

TEST_F(TSPSolverTest, FindOptimalSequenceTime_TwoTasks) {
    // ARRANGE: Two tasks where order matters
    Robot robot(0, {0.0, 0.0}, 100.0, -1, 1.0, 2.0);
    std::vector<Task> tasks = {
        Task(1, 0, 1),  // From 0 to 1
        Task(2, 1, 2)   // From 1 to 2
    };

    // ACT
    double time = TSPSolver::findOptimalSequenceTime(robot, tasks, simpleGraph);

    // ASSERT
    ASSERT_GT(time, 0.0) << "Two tasks should take some time";
    // The optimal order should be calculated
}

TEST_F(TSPSolverTest, FindOptimalSequenceTime_ThreeTasks) {
    // ARRANGE: Three tasks - TSP solver will try all 6 permutations
    Robot robot(0, {0.0, 0.0}, 100.0, -1, 1.0, 2.0);
    std::vector<Task> tasks = {
        Task(1, 0, 1),
        Task(2, 1, 2),
        Task(3, 2, 3)
    };

    // ACT
    double time = TSPSolver::findOptimalSequenceTime(robot, tasks, simpleGraph);

    // ASSERT
    ASSERT_GT(time, 0.0) << "Three tasks should take some time";
    // With 3 tasks starting at origin (0,0), optimal route will be found
}

// =============================================================================
// UNIT TESTS: BatteryConfig
// =============================================================================

class BatteryConfigTest : public ::testing::Test {
protected:
    Robot createTestRobot() {
        return Robot(0, {0.0, 0.0}, 100.0, -1, 1.6, 2.0);
    }
};

TEST_F(BatteryConfigTest, Constructor_InitializesFromRobot) {
    // ARRANGE
    Robot testRobot = createTestRobot();
    
    // ACT
    SchedulerUtils::BatteryConfig config(testRobot, 1.6);

    // ASSERT
    ASSERT_EQ(config.robotSpeed, 1.6);
    ASSERT_EQ(config.batteryRechargeRate, 2.0);
    ASSERT_EQ(config.fullBattery, 100.0);
    ASSERT_GT(config.lowBatteryThreshold, 0.0);
}

TEST_F(BatteryConfigTest, Constructor_DifferentSpeed) {
    // ARRANGE
    Robot testRobot = createTestRobot();
    
    // ACT
    SchedulerUtils::BatteryConfig config1(testRobot, 1.6);
    SchedulerUtils::BatteryConfig config2(testRobot, 2.0);

    // ASSERT
    ASSERT_NE(config1.robotSpeed, config2.robotSpeed);
}
