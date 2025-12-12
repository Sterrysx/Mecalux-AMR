/**
 * @file TaskLoader.cc
 * @brief Implementation of TaskLoader for JSON parsing and validation
 * 
 * Supports both numeric node IDs and string-based POI IDs (P1, P2, etc.)
 */

#include "../include/TaskLoader.hh"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>

namespace Backend {
namespace Layer2 {

// =============================================================================
// PUBLIC METHODS
// =============================================================================

std::vector<Task> TaskLoader::LoadTasksWithPOI(const std::string& filepath,
                                                const Backend::Layer1::NavMesh& mesh,
                                                const Backend::Layer1::POIRegistry& poiRegistry) {
    std::vector<Task> tasks;
    
    // Read file content
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[TaskLoader] ERROR: Cannot open file: " << filepath << std::endl;
        return tasks;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Parse JSON with POI resolution
    tasks = ParseJSONWithPOI(content, poiRegistry);
    
    // Validate against NavMesh
    return ValidateTasks(tasks, mesh);
}

std::vector<Task> TaskLoader::LoadTasks(const std::string& filepath,
                                        const Backend::Layer1::NavMesh& mesh) {
    std::vector<Task> tasks;
    
    // Read file content
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[TaskLoader] ERROR: Cannot open file: " << filepath << std::endl;
        return tasks;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Parse JSON
    tasks = ParseJSON(content);
    
    // Validate against NavMesh
    return ValidateTasks(tasks, mesh);
}

std::vector<Task> TaskLoader::ValidateTasks(const std::vector<Task>& tasks,
                                            const Backend::Layer1::NavMesh& mesh) {
    std::vector<Task> validTasks;
    validTasks.reserve(tasks.size());
    
    int invalidCount = 0;
    
    for (const auto& task : tasks) {
        bool sourceValid = IsValidNodeId(task.sourceNode, mesh);
        bool destValid = IsValidNodeId(task.destinationNode, mesh);
        
        if (sourceValid && destValid) {
            validTasks.push_back(task);
        } else {
            invalidCount++;
            std::cerr << "[TaskLoader] WARNING: Task " << task.taskId 
                      << " has invalid nodes (source=" << task.sourceNode
                      << " valid=" << sourceValid
                      << ", dest=" << task.destinationNode 
                      << " valid=" << destValid << ")" << std::endl;
        }
    }
    
    std::cout << "[TaskLoader] Validated " << validTasks.size() 
              << "/" << tasks.size() << " tasks";
    if (invalidCount > 0) {
        std::cout << " (" << invalidCount << " rejected)";
    }
    std::cout << std::endl;
    
    return validTasks;
}

bool TaskLoader::GenerateSampleTasksJSON(const std::string& filepath,
                                         const std::vector<int>& pickupNodes,
                                         const std::vector<int>& dropoffNodes,
                                         int numTasks) {
    if (pickupNodes.empty() || dropoffNodes.empty()) {
        std::cerr << "[TaskLoader] ERROR: Cannot generate tasks without pickup/dropoff nodes" << std::endl;
        return false;
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[TaskLoader] ERROR: Cannot create file: " << filepath << std::endl;
        return false;
    }
    
    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> pickupDist(0, pickupNodes.size() - 1);
    std::uniform_int_distribution<> dropoffDist(0, dropoffNodes.size() - 1);
    
    // Generate JSON
    file << "{\n";
    file << "  \"description\": \"Auto-generated sample tasks for testing\",\n";
    file << "  \"tasks\": [\n";
    
    for (int i = 0; i < numTasks; ++i) {
        int pickup = pickupNodes[pickupDist(gen)];
        int dropoff = dropoffNodes[dropoffDist(gen)];
        
        file << "    {\n";
        file << "      \"id\": " << (i + 1) << ",\n";
        file << "      \"pickup\": " << pickup << ",\n";
        file << "      \"dropoff\": " << dropoff << "\n";
        file << "    }";
        
        if (i < numTasks - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    
    std::cout << "[TaskLoader] Generated " << numTasks << " sample tasks in: " 
              << filepath << std::endl;
    
    return true;
}

// =============================================================================
// PRIVATE METHODS
// =============================================================================

bool TaskLoader::IsValidNodeId(int nodeId, const Backend::Layer1::NavMesh& mesh) {
    // Check if node exists in NavMesh
    const auto& allNodes = mesh.GetAllNodes();
    return nodeId >= 0 && nodeId < static_cast<int>(allNodes.size());
}

std::vector<Task> TaskLoader::ParseJSON(const std::string& content) {
    std::vector<Task> tasks;
    
    // Find "tasks" array
    size_t tasksPos = content.find("\"tasks\"");
    if (tasksPos == std::string::npos) {
        std::cerr << "[TaskLoader] ERROR: No 'tasks' array found in JSON" << std::endl;
        return tasks;
    }
    
    // Find opening bracket of array
    size_t arrayStart = content.find('[', tasksPos);
    if (arrayStart == std::string::npos) {
        std::cerr << "[TaskLoader] ERROR: Malformed JSON - no array start" << std::endl;
        return tasks;
    }
    
    // Find closing bracket
    size_t arrayEnd = content.rfind(']');
    if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        std::cerr << "[TaskLoader] ERROR: Malformed JSON - no array end" << std::endl;
        return tasks;
    }
    
    // Parse each task object
    size_t pos = arrayStart;
    while (pos < arrayEnd) {
        // Find next object start
        size_t objStart = content.find('{', pos);
        if (objStart == std::string::npos || objStart >= arrayEnd) break;
        
        // Find object end
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos || objEnd >= arrayEnd) break;
        
        std::string obj = content.substr(objStart, objEnd - objStart + 1);
        
        // Extract fields
        int id = -1, source = -1, dest = -1;
        
        // Parse "id"
        size_t idPos = obj.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colonPos = obj.find(':', idPos);
            if (colonPos != std::string::npos) {
                id = std::atoi(obj.c_str() + colonPos + 1);
            }
        }
        
        // Parse "source"
        size_t sourcePos = obj.find("\"source\"");
        if (sourcePos != std::string::npos) {
            size_t colonPos = obj.find(':', sourcePos);
            if (colonPos != std::string::npos) {
                source = std::atoi(obj.c_str() + colonPos + 1);
            }
        }
        
        // Parse "destination"
        size_t destPos = obj.find("\"destination\"");
        if (destPos != std::string::npos) {
            size_t colonPos = obj.find(':', destPos);
            if (colonPos != std::string::npos) {
                dest = std::atoi(obj.c_str() + colonPos + 1);
            }
        }
        
        // Create task if all fields are valid
        if (id >= 0 && source >= 0 && dest >= 0) {
            tasks.emplace_back(id, source, dest);
        } else {
            std::cerr << "[TaskLoader] WARNING: Skipping malformed task object" << std::endl;
        }
        
        pos = objEnd + 1;
    }
    
    std::cout << "[TaskLoader] Parsed " << tasks.size() << " tasks from JSON" << std::endl;
    
    return tasks;
}

std::vector<Task> TaskLoader::ParseJSONWithPOI(const std::string& content,
                                                const Backend::Layer1::POIRegistry& poiRegistry) {
    std::vector<Task> tasks;
    
    // Find "tasks" array
    size_t tasksPos = content.find("\"tasks\"");
    if (tasksPos == std::string::npos) {
        std::cerr << "[TaskLoader] ERROR: No 'tasks' array found in JSON" << std::endl;
        return tasks;
    }
    
    // Find opening bracket of array
    size_t arrayStart = content.find('[', tasksPos);
    if (arrayStart == std::string::npos) {
        std::cerr << "[TaskLoader] ERROR: Malformed JSON - no array start" << std::endl;
        return tasks;
    }
    
    // Find closing bracket
    size_t arrayEnd = content.rfind(']');
    if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        std::cerr << "[TaskLoader] ERROR: Malformed JSON - no array end" << std::endl;
        return tasks;
    }
    
    // Parse each task object
    size_t pos = arrayStart;
    while (pos < arrayEnd) {
        // Find next object start
        size_t objStart = content.find('{', pos);
        if (objStart == std::string::npos || objStart >= arrayEnd) break;
        
        // Find object end
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos || objEnd >= arrayEnd) break;
        
        std::string obj = content.substr(objStart, objEnd - objStart + 1);
        
        // Extract fields
        int id = -1;
        std::string sourceId, destId;
        int sourceNode = -1, destNode = -1;
        
        // Parse "id"
        size_t idPos = obj.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colonPos = obj.find(':', idPos);
            if (colonPos != std::string::npos) {
                id = std::atoi(obj.c_str() + colonPos + 1);
            }
        }
        
        // Parse "pickup" or "source" (string POI ID) - support both field names
        size_t sourcePos = obj.find("\"pickup\"");
        if (sourcePos == std::string::npos) {
            sourcePos = obj.find("\"source\"");  // Fallback to old format
        }
        if (sourcePos != std::string::npos) {
            size_t colonPos = obj.find(':', sourcePos);
            if (colonPos != std::string::npos) {
                // Find the string value in quotes
                size_t quoteStart = obj.find('"', colonPos);
                if (quoteStart != std::string::npos) {
                    size_t quoteEnd = obj.find('"', quoteStart + 1);
                    if (quoteEnd != std::string::npos) {
                        sourceId = obj.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
            }
        }
        
        // Parse "dropoff" or "destination" (string POI ID) - support both field names
        size_t destPos = obj.find("\"dropoff\"");
        if (destPos == std::string::npos) {
            destPos = obj.find("\"destination\"");  // Fallback to old format
        }
        if (destPos != std::string::npos) {
            size_t colonPos = obj.find(':', destPos);
            if (colonPos != std::string::npos) {
                // Find the string value in quotes
                size_t quoteStart = obj.find('"', colonPos);
                if (quoteStart != std::string::npos) {
                    size_t quoteEnd = obj.find('"', quoteStart + 1);
                    if (quoteEnd != std::string::npos) {
                        destId = obj.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
            }
        }
        
        // Resolve POI IDs to node IDs
        if (!sourceId.empty()) {
            sourceNode = poiRegistry.GetNodeForPOI(sourceId);
            if (sourceNode < 0) {
                std::cerr << "[TaskLoader] WARNING: Unknown POI '" << sourceId 
                          << "' for task " << id << std::endl;
            }
        }
        if (!destId.empty()) {
            destNode = poiRegistry.GetNodeForPOI(destId);
            if (destNode < 0) {
                std::cerr << "[TaskLoader] WARNING: Unknown POI '" << destId 
                          << "' for task " << id << std::endl;
            }
        }
        
        // Create task if all fields are valid
        if (id >= 0 && sourceNode >= 0 && destNode >= 0) {
            tasks.emplace_back(id, sourceNode, destNode, sourceId, destId);
        } else {
            std::cerr << "[TaskLoader] WARNING: Skipping task " << id 
                      << " (source='" << sourceId << "'→" << sourceNode 
                      << ", dest='" << destId << "'→" << destNode << ")" << std::endl;
        }
        
        pos = objEnd + 1;
    }
    
    std::cout << "[TaskLoader] Parsed " << tasks.size() << " tasks from JSON (POI format)" << std::endl;
    
    return tasks;
}

} // namespace Layer2
} // namespace Backend
