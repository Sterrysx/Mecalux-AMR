/**
 * @file TaskLoader.hh
 * @brief Task loading and validation for Layer 2 Fleet Manager
 * 
 * Parses task definitions from JSON and validates against NavMesh.
 * Supports string-based POI IDs (P1, P2, etc.) resolved via POIRegistry.
 */

#ifndef LAYER2_TASKLOADER_HH
#define LAYER2_TASKLOADER_HH

#include "Task.hh"
#include "../../layer1/include/NavMesh.hh"
#include "../../layer1/include/POIRegistry.hh"
#include <vector>
#include <string>
#include <set>

namespace Backend {
namespace Layer2 {

/**
 * @brief Loads and validates tasks from JSON configuration.
 * 
 * Expected JSON format (with string POI IDs):
 * {
 *   "tasks": [
 *     {
 *       "id": 1,
 *       "source": "P2",       // POI ID (resolved via POIRegistry)
 *       "destination": "P1"
 *     },
 *     ...
 *   ]
 * }
 * 
 * String POI IDs are resolved to NavMesh node IDs via POIRegistry.
 */
class TaskLoader {
public:
    /**
     * @brief Load tasks from a JSON file using POIRegistry for name resolution.
     * 
     * This is the preferred method for loading tasks. It:
     * 1. Parses the JSON file
     * 2. Resolves string POI IDs (e.g., "P1", "P2") to NavMesh node IDs
     * 3. Validates that all resolved node IDs exist in the NavMesh
     * 4. Returns only valid tasks
     * 
     * @param filepath Path to the JSON configuration file
     * @param mesh The NavMesh to validate node IDs against
     * @param poiRegistry The POIRegistry for resolving string IDs to node IDs
     * @return Vector of valid Task objects (invalid tasks are skipped with warnings)
     */
    static std::vector<Task> LoadTasksWithPOI(const std::string& filepath, 
                                              const Backend::Layer1::NavMesh& mesh,
                                              const Backend::Layer1::POIRegistry& poiRegistry);
    
    /**
     * @brief Load tasks from a JSON file and validate against NavMesh.
     * 
     * Legacy method - expects numeric node IDs in JSON.
     * 
     * @param filepath Path to the JSON configuration file
     * @param mesh The NavMesh to validate node IDs against
     * @return Vector of valid Task objects (invalid tasks are skipped with warnings)
     */
    static std::vector<Task> LoadTasks(const std::string& filepath, 
                                       const Backend::Layer1::NavMesh& mesh);

    /**
     * @brief Create tasks programmatically (for testing).
     * 
     * @param tasks Vector of tasks to validate
     * @param mesh The NavMesh to validate node IDs against
     * @return Vector of valid Task objects
     */
    static std::vector<Task> ValidateTasks(const std::vector<Task>& tasks,
                                           const Backend::Layer1::NavMesh& mesh);

    /**
     * @brief Create a sample tasks JSON file for testing.
     * 
     * @param filepath Path to create the JSON file
     * @param pickupNodes Vector of valid pickup node IDs
     * @param dropoffNodes Vector of valid dropoff node IDs
     * @param numTasks Number of random tasks to generate
     * @return true if file was created successfully
     */
    static bool GenerateSampleTasksJSON(const std::string& filepath,
                                        const std::vector<int>& pickupNodes,
                                        const std::vector<int>& dropoffNodes,
                                        int numTasks);

private:
    /**
     * @brief Check if a node ID exists in the NavMesh.
     */
    static bool IsValidNodeId(int nodeId, const Backend::Layer1::NavMesh& mesh);

    /**
     * @brief Parse JSON content and extract tasks (legacy numeric format).
     */
    static std::vector<Task> ParseJSON(const std::string& content);

    /**
     * @brief Parse JSON content and extract tasks with string POI IDs.
     * 
     * @param content JSON string content
     * @param poiRegistry Registry for resolving string IDs to node IDs
     * @return Vector of tasks with resolved node IDs
     */
    static std::vector<Task> ParseJSONWithPOI(const std::string& content,
                                              const Backend::Layer1::POIRegistry& poiRegistry);
};

} // namespace Layer2
} // namespace Backend

#endif // LAYER2_TASKLOADER_HH
