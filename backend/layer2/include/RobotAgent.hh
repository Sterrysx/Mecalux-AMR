/**
 * @file RobotAgent.hh
 * @brief Robot state management for Layer 2 Fleet Manager
 * 
 * Defines robot status, state, and the agent abstraction for fleet management.
 * 
 * Battery System:
 * - Full battery: 300 seconds of operation
 * - Low battery threshold: 20% → go to charging zone
 * - Charging time: 60 seconds (0% to 100%)
 * - Smart check: At 27% battery, if task needs >7% of battery, go charge first
 */

#ifndef LAYER2_ROBOTAGENT_HH
#define LAYER2_ROBOTAGENT_HH

#include "../../common/include/Coordinates.hh"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace Backend {
namespace Layer2 {

// =============================================================================
// BATTERY CONSTANTS
// =============================================================================
constexpr float BATTERY_FULL_SECONDS = 300.0f;      ///< Full battery lasts 300 seconds
constexpr float BATTERY_CHARGE_TIME = 60.0f;        ///< Time to charge from 0% to 100%
constexpr float BATTERY_LOW_THRESHOLD = 0.20f;      ///< Go charge at 20%
constexpr float BATTERY_SMART_THRESHOLD = 0.27f;    ///< Smart check threshold (27%)
constexpr float BATTERY_SMART_MARGIN = 0.07f;       ///< If task needs >7%, go charge

/**
 * @brief Robot operational status.
 */
enum class RobotStatus {
    IDLE,       ///< Robot is available for new tasks
    BUSY,       ///< Robot is executing tasks (following itinerary)
    CHARGING,   ///< Robot is at charging station
    ERROR       ///< Robot has encountered an error
};

/**
 * @brief Convert RobotStatus to string for debugging.
 */
inline std::string StatusToString(RobotStatus status) {
    switch (status) {
        case RobotStatus::IDLE:     return "IDLE";
        case RobotStatus::BUSY:     return "BUSY";
        case RobotStatus::CHARGING: return "CHARGING";
        case RobotStatus::ERROR:    return "ERROR";
        default:                     return "UNKNOWN";
    }
}

/**
 * @brief Current state of a robot (mutable data).
 * 
 * This struct holds the dynamic state that changes as the robot operates.
 * It is updated by the simulator/real hardware via the FleetManager.
 */
struct RobotState {
    int currentNodeId;                  ///< Current NavMesh node (or nearest)
    RobotStatus status;                 ///< Current operational status
    float currentBatteryLevel;          ///< Battery level (0.0 - 1.0, percentage)
    std::vector<int> currentItinerary;  ///< Ordered list of goal node IDs
    
    /**
     * @brief Default constructor (idle at node 0 with full battery).
     */
    RobotState()
        : currentNodeId(0)
        , status(RobotStatus::IDLE)
        , currentBatteryLevel(1.0f)
        , currentItinerary() {}
    
    /**
     * @brief Get the next goal from the itinerary (without removing it).
     * @return Next goal node ID, or -1 if itinerary is empty
     */
    int PeekNextGoal() const {
        return currentItinerary.empty() ? -1 : currentItinerary.front();
    }
    
    /**
     * @brief Remove and return the next goal from the itinerary.
     * @return Next goal node ID, or -1 if itinerary is empty
     */
    int PopNextGoal() {
        if (currentItinerary.empty()) return -1;
        int goal = currentItinerary.front();
        currentItinerary.erase(currentItinerary.begin());
        return goal;
    }
    
    /**
     * @brief Check if the robot has pending goals.
     */
    bool HasPendingGoals() const {
        return !currentItinerary.empty();
    }
};

/**
 * @brief Robot agent abstraction for fleet management.
 * 
 * Each RobotAgent represents a physical robot in the fleet.
 * It maintains:
 * - Static configuration (ID, capacity, speed, charging station)
 * - Dynamic state (position, battery, itinerary)
 * 
 * The Fleet Manager owns all RobotAgents and coordinates their assignments.
 */
class RobotAgent {
private:
    // --- Static Configuration (set at construction) ---
    int robotId_;                       ///< Unique robot identifier
    float totalBatteryCapacity_;        ///< Battery capacity in kWh (or normalized to 1.0)
    int chargingStationNodeId_;         ///< Assigned charging station (NavMesh node)
    float averageSpeed_mps_;            ///< Average speed in meters per second
    int capacity_;                      ///< Load capacity (packets at a time)
    
    // --- Dynamic State ---
    RobotState currentState_;           ///< Mutable robot state

public:
    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Default constructor.
     */
    RobotAgent()
        : robotId_(-1)
        , totalBatteryCapacity_(1.0f)
        , chargingStationNodeId_(-1)
        , averageSpeed_mps_(1.0f)
        , capacity_(1)
        , currentState_() {}
    
    /**
     * @brief Parameterized constructor.
     * 
     * @param id Unique robot identifier
     * @param batteryCapacity Total battery capacity (normalized)
     * @param chargingNode Assigned charging station node ID
     * @param speed Average speed in m/s
     * @param loadCapacity Number of packets the robot can carry
     */
    RobotAgent(int id, float batteryCapacity, int chargingNode, 
               float speed, int loadCapacity)
        : robotId_(id)
        , totalBatteryCapacity_(batteryCapacity)
        , chargingStationNodeId_(chargingNode)
        , averageSpeed_mps_(speed)
        , capacity_(loadCapacity)
        , currentState_() {}

    // =========================================================================
    // GETTERS - Static Configuration
    // =========================================================================
    
    int GetRobotId() const { return robotId_; }
    float GetBatteryCapacity() const { return totalBatteryCapacity_; }
    int GetChargingStationNode() const { return chargingStationNodeId_; }
    float GetAverageSpeed() const { return averageSpeed_mps_; }
    int GetCapacity() const { return capacity_; }

    // =========================================================================
    // GETTERS - Dynamic State
    // =========================================================================
    
    const RobotState& GetState() const { return currentState_; }
    RobotState& GetMutableState() { return currentState_; }
    
    int GetCurrentNodeId() const { return currentState_.currentNodeId; }
    RobotStatus GetStatus() const { return currentState_.status; }
    float GetCurrentBattery() const { return currentState_.currentBatteryLevel; }
    const std::vector<int>& GetItinerary() const { return currentState_.currentItinerary; }

    // =========================================================================
    // SETTERS - Dynamic State
    // =========================================================================
    
    void SetCurrentNodeId(int nodeId) { currentState_.currentNodeId = nodeId; }
    void SetStatus(RobotStatus status) { currentState_.status = status; }
    void SetBatteryLevel(float level) { currentState_.currentBatteryLevel = level; }

    // =========================================================================
    // ITINERARY MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Assign a new itinerary to the robot.
     * 
     * This replaces the current itinerary with a new one.
     * Called by the VRP solver when (re)assigning tasks.
     * 
     * @param nodes Ordered list of goal node IDs to visit
     */
    void AssignItinerary(const std::vector<int>& nodes) {
        currentState_.currentItinerary = nodes;
        if (!nodes.empty() && currentState_.status == RobotStatus::IDLE) {
            currentState_.status = RobotStatus::BUSY;
        }
    }
    
    /**
     * @brief Append goals to the existing itinerary.
     * 
     * Used for incremental task insertion.
     * 
     * @param nodes Goals to append
     */
    void AppendToItinerary(const std::vector<int>& nodes) {
        currentState_.currentItinerary.insert(
            currentState_.currentItinerary.end(),
            nodes.begin(), nodes.end()
        );
        if (!currentState_.currentItinerary.empty() && 
            currentState_.status == RobotStatus::IDLE) {
            currentState_.status = RobotStatus::BUSY;
        }
    }
    
    /**
     * @brief Clear the current itinerary.
     */
    void ClearItinerary() {
        currentState_.currentItinerary.clear();
    }
    
    /**
     * @brief Get the next goal node from the itinerary.
     * @return Next goal node ID, or -1 if none
     */
    int GetNextGoal() const {
        return currentState_.PeekNextGoal();
    }
    
    /**
     * @brief Mark the current goal as completed and move to next.
     * 
     * Called when the robot arrives at a goal node.
     * If no more goals, transitions to IDLE.
     */
    void CompleteCurrentGoal() {
        currentState_.PopNextGoal();
        if (!currentState_.HasPendingGoals()) {
            currentState_.status = RobotStatus::IDLE;
        }
    }

    // =========================================================================
    // STATE UPDATE
    // =========================================================================
    
    /**
     * @brief Update robot state from external source (simulator/hardware).
     * 
     * @param nodeId Current position (nearest NavMesh node)
     * @param batteryLevel Current battery level (0.0 - 1.0)
     */
    void UpdateState(int nodeId, float batteryLevel) {
        currentState_.currentNodeId = nodeId;
        currentState_.currentBatteryLevel = batteryLevel;
        
        // Auto-transition to CHARGING if at charging station with low battery
        if (nodeId == chargingStationNodeId_ && batteryLevel < 0.95f) {
            currentState_.status = RobotStatus::CHARGING;
        }
        
        // Check for critical battery
        if (batteryLevel < 0.1f && currentState_.status != RobotStatus::CHARGING) {
            // Insert charging station as next goal
            if (chargingStationNodeId_ >= 0) {
                currentState_.currentItinerary.insert(
                    currentState_.currentItinerary.begin(),
                    chargingStationNodeId_
                );
            }
        }
    }

    // =========================================================================
    // COST ESTIMATION
    // =========================================================================
    
    /**
     * @brief Estimate time to complete current itinerary.
     * 
     * This is a rough estimate based on Euclidean distance.
     * For accurate costs, use CostMatrixProvider.
     * 
     * @param getCost Function to get cost between two nodes
     * @return Estimated time in seconds
     */
    template<typename CostFunc>
    float EstimateCompletionTime(CostFunc getCost) const {
        if (currentState_.currentItinerary.empty()) return 0.0f;
        
        float totalCost = 0.0f;
        int prevNode = currentState_.currentNodeId;
        
        for (int goalNode : currentState_.currentItinerary) {
            totalCost += getCost(prevNode, goalNode);
            prevNode = goalNode;
        }
        
        return totalCost;
    }

    // =========================================================================
    // UTILITY
    // =========================================================================
    
    /**
     * @brief Check if robot is available for new tasks.
     */
    bool IsAvailable() const {
        return currentState_.status == RobotStatus::IDLE;
    }
    
    /**
     * @brief Check if robot needs charging (battery ≤ 20%).
     */
    bool NeedsCharging() const {
        return currentState_.currentBatteryLevel <= BATTERY_LOW_THRESHOLD;
    }
    
    // =========================================================================
    // BATTERY MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Get battery level as remaining seconds of operation.
     */
    float GetBatterySeconds() const {
        return currentState_.currentBatteryLevel * BATTERY_FULL_SECONDS;
    }
    
    /**
     * @brief Convert battery percentage to operation time in seconds.
     */
    static float BatteryToSeconds(float batteryLevel) {
        return batteryLevel * BATTERY_FULL_SECONDS;
    }
    
    /**
     * @brief Convert operation time in seconds to battery percentage.
     */
    static float SecondsToBattery(float seconds) {
        return seconds / BATTERY_FULL_SECONDS;
    }
    
    /**
     * @brief Consume battery for a given travel time.
     * 
     * @param travelTimeSeconds Time spent traveling
     * @return Remaining battery level (0.0-1.0)
     */
    float ConsumeBattery(float travelTimeSeconds) {
        float consumption = SecondsToBattery(travelTimeSeconds);
        currentState_.currentBatteryLevel = std::max(0.0f, 
            currentState_.currentBatteryLevel - consumption);
        return currentState_.currentBatteryLevel;
    }
    
    /**
     * @brief Charge the battery for a given time.
     * 
     * @param chargeTimeSeconds Time spent charging
     * @return New battery level (0.0-1.0)
     */
    float ChargeBattery(float chargeTimeSeconds) {
        // Charge rate: 100% in 60 seconds = 1.667% per second
        float chargeRate = 1.0f / BATTERY_CHARGE_TIME;
        float charged = chargeTimeSeconds * chargeRate;
        currentState_.currentBatteryLevel = std::min(1.0f, 
            currentState_.currentBatteryLevel + charged);
        return currentState_.currentBatteryLevel;
    }
    
    /**
     * @brief Get the time needed to fully charge the battery.
     */
    float GetTimeToFullCharge() const {
        float deficit = 1.0f - currentState_.currentBatteryLevel;
        return deficit * BATTERY_CHARGE_TIME;
    }
    
    /**
     * @brief Check if robot should charge before attempting a task.
     * 
     * Smart charging logic:
     * - If battery ≤ 20%, MUST charge
     * - If battery ≤ 27% and task requires > 7% battery, SHOULD charge
     * 
     * @param taskTimeSeconds Estimated time for the task
     * @return true if robot should go to charging station first
     */
    bool ShouldChargeBeforeTask(float taskTimeSeconds) const {
        // Critical: Must charge if at or below 20%
        if (currentState_.currentBatteryLevel <= BATTERY_LOW_THRESHOLD) {
            return true;
        }
        
        // Smart check: At 27% or less, check if task needs > 7%
        if (currentState_.currentBatteryLevel <= BATTERY_SMART_THRESHOLD) {
            float taskBatteryNeeded = SecondsToBattery(taskTimeSeconds);
            if (taskBatteryNeeded > BATTERY_SMART_MARGIN) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Check if robot can complete a task with current battery.
     * 
     * @param taskTimeSeconds Estimated time for the task
     * @param returnToChargerSeconds Time to return to charger after task
     * @return true if robot has enough battery
     */
    bool CanCompleteTask(float taskTimeSeconds, float returnToChargerSeconds = 0.0f) const {
        float totalNeeded = SecondsToBattery(taskTimeSeconds + returnToChargerSeconds);
        // Keep 5% safety margin
        return (currentState_.currentBatteryLevel - totalNeeded) >= 0.05f;
    }
    
    /**
     * @brief Stream output operator for debugging.
     */
    friend std::ostream& operator<<(std::ostream& os, const RobotAgent& agent) {
        os << "Robot[id=" << agent.robotId_ 
           << ", status=" << StatusToString(agent.currentState_.status)
           << ", node=" << agent.currentState_.currentNodeId
           << ", battery=" << static_cast<int>(agent.currentState_.currentBatteryLevel * 100) << "%"
           << " (" << std::fixed << std::setprecision(1) << agent.GetBatterySeconds() << "s)"
           << ", goals=" << agent.currentState_.currentItinerary.size() << "]";
        return os;
    }
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_ROBOTAGENT_HH
