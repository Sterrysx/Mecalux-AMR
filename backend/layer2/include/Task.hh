/**
 * @file Task.hh
 * @brief Task definition for Layer 2 Fleet Manager
 * 
 * A Task represents a packet pickup/dropoff operation.
 * Tasks are loaded from JSON and validated against NavMesh nodes.
 */

#ifndef LAYER2_TASK_HH
#define LAYER2_TASK_HH

#include <string>
#include <iostream>

namespace Backend {
namespace Layer2 {

/**
 * @brief Represents a pickup/dropoff task in the warehouse.
 * 
 * Each task has:
 * - A unique identifier (from the external system)
 * - A source node (PICKUP POI) where the packet is picked up
 * - A destination node (DROPOFF POI) where the packet is delivered
 * 
 * The nodes are NavMesh node IDs, validated during loading.
 * Optional string IDs store the original POI names (P1, P2, etc.)
 */
struct Task {
    int taskId;              ///< Unique task identifier
    int sourceNode;          ///< NavMesh node ID for pickup location
    int destinationNode;     ///< NavMesh node ID for dropoff location
    std::string sourceId;    ///< Original POI ID string (e.g., "P2")
    std::string destId;      ///< Original POI ID string (e.g., "P1")
    
    /**
     * @brief Default constructor.
     */
    Task() : taskId(-1), sourceNode(-1), destinationNode(-1), sourceId(""), destId("") {}
    
    /**
     * @brief Parameterized constructor (node IDs only).
     * 
     * @param id Unique task identifier
     * @param source NavMesh node ID for pickup
     * @param dest NavMesh node ID for dropoff
     */
    Task(int id, int source, int dest) 
        : taskId(id), sourceNode(source), destinationNode(dest), sourceId(""), destId("") {}
    
    /**
     * @brief Full constructor with POI ID strings.
     * 
     * @param id Unique task identifier
     * @param source NavMesh node ID for pickup
     * @param dest NavMesh node ID for dropoff
     * @param srcId Original source POI ID string
     * @param dstId Original destination POI ID string
     */
    Task(int id, int source, int dest, const std::string& srcId, const std::string& dstId) 
        : taskId(id), sourceNode(source), destinationNode(dest), sourceId(srcId), destId(dstId) {}
    
    /**
     * @brief Comparison operator for sorting.
     */
    bool operator<(const Task& other) const {
        return taskId < other.taskId;
    }
    
    /**
     * @brief Equality operator.
     */
    bool operator==(const Task& other) const {
        return taskId == other.taskId;
    }
    
    /**
     * @brief Check if the task is valid (has valid node IDs).
     */
    bool IsValid() const {
        return taskId >= 0 && sourceNode >= 0 && destinationNode >= 0;
    }
    
    /**
     * @brief Stream output operator for debugging.
     */
    friend std::ostream& operator<<(std::ostream& os, const Task& task) {
        os << "Task{id=" << task.taskId;
        if (!task.sourceId.empty()) {
            os << ", source=" << task.sourceId << "(" << task.sourceNode << ")";
            os << ", dest=" << task.destId << "(" << task.destinationNode << ")";
        } else {
            os << ", source=" << task.sourceNode 
               << ", dest=" << task.destinationNode;
        }
        os << "}";
        return os;
    }
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_TASK_HH
