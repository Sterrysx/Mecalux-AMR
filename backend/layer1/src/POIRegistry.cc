#include "POIRegistry.hh"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Backend {
namespace Layer1 {

    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================

    POIRegistry::POIRegistry() : isMappedToNavMesh(false) {}

    // =========================================================================
    // TYPE CONVERSION UTILITIES
    // =========================================================================

    std::string POIRegistry::TypeToString(POIType type) {
        switch (type) {
            case POIType::CHARGING: return "CHARGING";
            case POIType::PICKUP:   return "PICKUP";
            case POIType::DROPOFF:  return "DROPOFF";
            default:                return "UNKNOWN";
        }
    }

    POIType POIRegistry::StringToType(const std::string& typeStr) {
        if (typeStr == "CHARGING" || typeStr == "charging" || 
            typeStr == "CHARGER" || typeStr == "charger")
            return POIType::CHARGING;
        if (typeStr == "PICKUP" || typeStr == "pickup")
            return POIType::PICKUP;
        if (typeStr == "DROPOFF" || typeStr == "dropoff")
            return POIType::DROPOFF;
        // Default to PICKUP for unknown packet-related types
        return POIType::PICKUP;
    }

    // =========================================================================
    // LOADING - Simple JSON Parser (no external dependencies)
    // =========================================================================

    // Simple helper to extract a string value from JSON-like text
    static std::string extractStringValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";
        
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return "";
        
        size_t startQuote = json.find('"', colonPos);
        if (startQuote == std::string::npos) return "";
        
        size_t endQuote = json.find('"', startQuote + 1);
        if (endQuote == std::string::npos) return "";
        
        return json.substr(startQuote + 1, endQuote - startQuote - 1);
    }

    // Simple helper to extract an integer value from JSON-like text
    static int extractIntValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return 0;
        
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return 0;
        
        // Skip whitespace
        size_t numStart = colonPos + 1;
        while (numStart < json.size() && (json[numStart] == ' ' || json[numStart] == '\t')) {
            ++numStart;
        }
        
        // Read the number
        std::string numStr;
        while (numStart < json.size() && (std::isdigit(json[numStart]) || json[numStart] == '-')) {
            numStr += json[numStart];
            ++numStart;
        }
        
        return numStr.empty() ? 0 : std::stoi(numStr);
    }

    // Simple helper to extract a boolean value from JSON-like text
    static bool extractBoolValue(const std::string& json, const std::string& key, bool defaultVal = true) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return defaultVal;
        
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return defaultVal;
        
        // Look for "true" or "false"
        size_t truePos = json.find("true", colonPos);
        size_t falsePos = json.find("false", colonPos);
        size_t nextComma = json.find(',', colonPos);
        size_t nextBrace = json.find('}', colonPos);
        
        size_t boundary = std::min(nextComma, nextBrace);
        
        if (truePos != std::string::npos && truePos < boundary) return true;
        if (falsePos != std::string::npos && falsePos < boundary) return false;
        
        return defaultVal;
    }

    bool POIRegistry::LoadFromJSON(const std::string& filepath) {
        std::cout << "[POIRegistry] Loading POI configuration from: " << filepath << std::endl;
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[POIRegistry] Failed to open file: " << filepath << std::endl;
            return false;
        }

        // Read entire file
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // Find POI array
        size_t poiArrayStart = content.find("\"poi\"");
        if (poiArrayStart == std::string::npos) {
            // Try alternative key names
            poiArrayStart = content.find("\"pois\"");
            if (poiArrayStart == std::string::npos) {
                poiArrayStart = content.find("\"points_of_interest\"");
            }
        }
        
        if (poiArrayStart == std::string::npos) {
            std::cerr << "[POIRegistry] No POI array found in JSON" << std::endl;
            return false;
        }

        // Find the start of the array
        size_t arrayStart = content.find('[', poiArrayStart);
        if (arrayStart == std::string::npos) {
            std::cerr << "[POIRegistry] Invalid JSON format - no array found" << std::endl;
            return false;
        }

        // Parse each POI object
        size_t pos = arrayStart;
        int loadedCount = 0;
        
        while (true) {
            size_t objStart = content.find('{', pos);
            if (objStart == std::string::npos) break;
            
            size_t objEnd = content.find('}', objStart);
            if (objEnd == std::string::npos) break;
            
            // Check if we've gone past the array end
            size_t arrayEnd = content.find(']', arrayStart);
            if (objStart > arrayEnd) break;
            
            // Extract this POI object
            std::string poiObj = content.substr(objStart, objEnd - objStart + 1);
            
            // Parse fields
            std::string id = extractStringValue(poiObj, "id");
            std::string typeStr = extractStringValue(poiObj, "type");
            int x = extractIntValue(poiObj, "x");
            int y = extractIntValue(poiObj, "y");
            bool active = extractBoolValue(poiObj, "active", true);
            
            if (!id.empty()) {
                POIType type = StringToType(typeStr);
                Backend::Common::Coordinates coords{x, y};
                
                // All types now map to CHARGING or PACKET
                AddPOI(id, type, coords, active);
                ++loadedCount;
            }
            
            pos = objEnd + 1;
        }

        std::cout << "[POIRegistry] Loaded " << loadedCount << " POIs" << std::endl;
        return loadedCount > 0;
    }

    // =========================================================================
    // ADDING POIS
    // =========================================================================

    int POIRegistry::AddPOI(const std::string& id, POIType type,
                            Backend::Common::Coordinates coords, bool active) {
        // Check for duplicate ID
        if (poiById.find(id) != poiById.end()) {
            std::cerr << "[POIRegistry] Duplicate POI ID: " << id << std::endl;
            return -1;
        }

        PointOfInterest poi;
        poi.id = id;
        poi.type = type;
        poi.typeName = TypeToString(type);
        poi.worldCoords = coords;
        poi.nearestNodeId = -1;  // Not mapped yet
        poi.distanceToNode = -1.0f;
        poi.isActive = active;

        int index = static_cast<int>(allPOIs.size());
        allPOIs.push_back(poi);
        
        // Update lookup tables
        poiById[id] = index;
        poiByType[type].push_back(index);
        poiByTypeName[poi.typeName].push_back(index);
        
        // Reset mapping flag since we added new POI
        isMappedToNavMesh = false;

        return index;
    }

    // AddCustomPOI maps unknown types to PICKUP by default
    int POIRegistry::AddCustomPOI(const std::string& id, const std::string& typeName,
                                   Backend::Common::Coordinates coords, bool active) {
        // Map custom types to PICKUP by default
        return AddPOI(id, POIType::PICKUP, coords, active);
    }

    // =========================================================================
    // NAVMESH MAPPING & SAFETY VALIDATION
    // =========================================================================

    int POIRegistry::ValidateAndMapToNavMesh(const NavMesh& mesh, const AbstractGrid& safetyMap, 
                                              float maxDistance) {
        std::cout << "[POIRegistry] Validating " << allPOIs.size() << " POIs against Safety Map..." << std::endl;
        
        // Clear existing node mappings
        poiByNodeId.clear();
        
        int successCount = 0;
        int failCount = 0;
        int disabledCount = 0;
        
        const auto& nodes = mesh.GetAllNodes();
        
        for (size_t i = 0; i < allPOIs.size(); ++i) {
            PointOfInterest& poi = allPOIs[i];
            
            // ===================================================================
            // CRITICAL SAFETY CHECK
            // If the POI coordinate is in the "Gray Zone" of the Inflated Map,
            // the robot physically cannot go there without clipping a wall.
            // ===================================================================
            if (!safetyMap.IsAccessible(poi.worldCoords)) {
                std::cerr << "[POIRegistry] \033[1;31mCRITICAL WARNING\033[0m: POI '" << poi.id 
                          << "' at (" << poi.worldCoords.x << "," << poi.worldCoords.y << ")"
                          << " is inside an Obstacle Inflation Zone!" << std::endl;
                std::cerr << "              Robot will crash if sent to this location." << std::endl;
                std::cerr << "              >>> AUTO-DISABLING POI <<<" << std::endl;
                
                poi.isActive = false;  // Disable it automatically
                poi.nearestNodeId = -1;
                poi.distanceToNode = -1.0f;
                ++disabledCount;
                continue;
            }
            
            // Find nearest node
            int bestNodeId = -1;
            float bestDist = std::numeric_limits<float>::max();
            
            for (size_t n = 0; n < nodes.size(); ++n) {
                float dist = poi.worldCoords.DistanceTo(nodes[n].coords);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestNodeId = static_cast<int>(n);
                }
            }
            
            // Check if within max distance (if specified)
            if (maxDistance > 0.0f && bestDist > maxDistance) {
                std::cerr << "[POIRegistry] Warning: POI '" << poi.id 
                          << "' is " << bestDist << " units from nearest node (max: " 
                          << maxDistance << ")" << std::endl;
                ++failCount;
                continue;
            }
            
            // Map the POI to the node
            poi.nearestNodeId = bestNodeId;
            poi.distanceToNode = bestDist;
            
            // Update reverse lookup
            poiByNodeId[bestNodeId].push_back(static_cast<int>(i));
            
            ++successCount;
        }
        
        isMappedToNavMesh = true;
        
        std::cout << "[POIRegistry] Validation complete:" << std::endl;
        std::cout << "              " << successCount << " POIs validated and mapped successfully" << std::endl;
        if (disabledCount > 0) {
            std::cout << "              \033[1;31m" << disabledCount << " POIs DISABLED (unsafe location)\033[0m" << std::endl;
        }
        if (failCount > 0) {
            std::cout << "              " << failCount << " POIs failed distance check" << std::endl;
        }
        
        return successCount;
    }

    int POIRegistry::MapToNavMesh(const NavMesh& mesh, float maxDistance) {
        std::cout << "[POIRegistry] Mapping " << allPOIs.size() << " POIs to NavMesh..." << std::endl;
        std::cout << "[POIRegistry] WARNING: No safety validation! Use ValidateAndMapToNavMesh() instead." << std::endl;
        
        // Clear existing node mappings
        poiByNodeId.clear();
        
        int successCount = 0;
        int failCount = 0;
        
        const auto& nodes = mesh.GetAllNodes();
        
        for (size_t i = 0; i < allPOIs.size(); ++i) {
            PointOfInterest& poi = allPOIs[i];
            
            // Find nearest node
            int bestNodeId = -1;
            float bestDist = std::numeric_limits<float>::max();
            
            for (size_t n = 0; n < nodes.size(); ++n) {
                float dist = poi.worldCoords.DistanceTo(nodes[n].coords);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestNodeId = static_cast<int>(n);
                }
            }
            
            // Check if within max distance (if specified)
            if (maxDistance > 0.0f && bestDist > maxDistance) {
                std::cerr << "[POIRegistry] Warning: POI '" << poi.id 
                          << "' is " << bestDist << " units from nearest node (max: " 
                          << maxDistance << ")" << std::endl;
                ++failCount;
                continue;
            }
            
            // Map the POI to the node
            poi.nearestNodeId = bestNodeId;
            poi.distanceToNode = bestDist;
            
            // Update reverse lookup
            poiByNodeId[bestNodeId].push_back(static_cast<int>(i));
            
            ++successCount;
        }
        
        isMappedToNavMesh = true;
        
        std::cout << "[POIRegistry] Mapped " << successCount << " POIs successfully";
        if (failCount > 0) {
            std::cout << " (" << failCount << " failed distance check)";
        }
        std::cout << std::endl;
        
        return successCount;
    }

    bool POIRegistry::IsMapped() const {
        return isMappedToNavMesh;
    }

    // =========================================================================
    // QUERIES BY TYPE
    // =========================================================================

    std::vector<int> POIRegistry::GetNodesByType(POIType type) const {
        std::vector<int> nodeIds;
        
        auto it = poiByType.find(type);
        if (it == poiByType.end()) return nodeIds;
        
        for (int poiIdx : it->second) {
            if (poiIdx >= 0 && poiIdx < static_cast<int>(allPOIs.size())) {
                int nodeId = allPOIs[poiIdx].nearestNodeId;
                if (nodeId >= 0) {
                    // Avoid duplicates if multiple POIs map to same node
                    if (std::find(nodeIds.begin(), nodeIds.end(), nodeId) == nodeIds.end()) {
                        nodeIds.push_back(nodeId);
                    }
                }
            }
        }
        
        return nodeIds;
    }

    std::vector<int> POIRegistry::GetNodesByTypeName(const std::string& typeName) const {
        std::vector<int> nodeIds;
        
        auto it = poiByTypeName.find(typeName);
        if (it == poiByTypeName.end()) return nodeIds;
        
        for (int poiIdx : it->second) {
            if (poiIdx >= 0 && poiIdx < static_cast<int>(allPOIs.size())) {
                int nodeId = allPOIs[poiIdx].nearestNodeId;
                if (nodeId >= 0) {
                    if (std::find(nodeIds.begin(), nodeIds.end(), nodeId) == nodeIds.end()) {
                        nodeIds.push_back(nodeId);
                    }
                }
            }
        }
        
        return nodeIds;
    }

    std::vector<const PointOfInterest*> POIRegistry::GetPOIsByType(POIType type) const {
        std::vector<const PointOfInterest*> result;
        
        auto it = poiByType.find(type);
        if (it == poiByType.end()) return result;
        
        for (int poiIdx : it->second) {
            if (poiIdx >= 0 && poiIdx < static_cast<int>(allPOIs.size())) {
                result.push_back(&allPOIs[poiIdx]);
            }
        }
        
        return result;
    }

    // =========================================================================
    // QUERIES BY NODE
    // =========================================================================

    std::vector<const PointOfInterest*> POIRegistry::GetPOIsAtNode(int nodeId) const {
        std::vector<const PointOfInterest*> result;
        
        auto it = poiByNodeId.find(nodeId);
        if (it == poiByNodeId.end()) return result;
        
        for (int poiIdx : it->second) {
            if (poiIdx >= 0 && poiIdx < static_cast<int>(allPOIs.size())) {
                result.push_back(&allPOIs[poiIdx]);
            }
        }
        
        return result;
    }

    bool POIRegistry::NodeHasPOIType(int nodeId, POIType type) const {
        auto pois = GetPOIsAtNode(nodeId);
        for (const auto* poi : pois) {
            if (poi->type == type) return true;
        }
        return false;
    }

    // =========================================================================
    // QUERIES BY ID
    // =========================================================================

    const PointOfInterest* POIRegistry::GetPOIById(const std::string& id) const {
        auto it = poiById.find(id);
        if (it == poiById.end()) return nullptr;
        
        int idx = it->second;
        if (idx < 0 || idx >= static_cast<int>(allPOIs.size())) return nullptr;
        
        return &allPOIs[idx];
    }

    int POIRegistry::GetNodeForPOI(const std::string& poiId) const {
        const PointOfInterest* poi = GetPOIById(poiId);
        return poi ? poi->nearestNodeId : -1;
    }

    // =========================================================================
    // STATE MANAGEMENT
    // =========================================================================

    bool POIRegistry::SetPOIActive(const std::string& id, bool active) {
        auto it = poiById.find(id);
        if (it == poiById.end()) return false;
        
        allPOIs[it->second].isActive = active;
        return true;
    }

    std::vector<int> POIRegistry::GetActiveNodesByType(POIType type) const {
        std::vector<int> nodeIds;
        
        auto it = poiByType.find(type);
        if (it == poiByType.end()) return nodeIds;
        
        for (int poiIdx : it->second) {
            if (poiIdx >= 0 && poiIdx < static_cast<int>(allPOIs.size())) {
                const PointOfInterest& poi = allPOIs[poiIdx];
                if (poi.isActive && poi.nearestNodeId >= 0) {
                    if (std::find(nodeIds.begin(), nodeIds.end(), poi.nearestNodeId) == nodeIds.end()) {
                        nodeIds.push_back(poi.nearestNodeId);
                    }
                }
            }
        }
        
        return nodeIds;
    }

    // =========================================================================
    // UTILITIES
    // =========================================================================

    size_t POIRegistry::GetPOICount() const {
        return allPOIs.size();
    }

    size_t POIRegistry::GetPOICountByType(POIType type) const {
        auto it = poiByType.find(type);
        return it != poiByType.end() ? it->second.size() : 0;
    }

    void POIRegistry::Clear() {
        allPOIs.clear();
        poiByType.clear();
        poiByTypeName.clear();
        poiByNodeId.clear();
        poiById.clear();
        isMappedToNavMesh = false;
    }

    bool POIRegistry::ExportToJSON(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[POIRegistry] Failed to open file for writing: " << filepath << std::endl;
            return false;
        }

        file << "{\n";
        file << "  \"poi\": [\n";
        
        for (size_t i = 0; i < allPOIs.size(); ++i) {
            const PointOfInterest& poi = allPOIs[i];
            
            file << "    {\n";
            file << "      \"id\": \"" << poi.id << "\",\n";
            file << "      \"type\": \"" << poi.typeName << "\",\n";
            file << "      \"x\": " << poi.worldCoords.x << ",\n";
            file << "      \"y\": " << poi.worldCoords.y << ",\n";
            file << "      \"active\": " << (poi.isActive ? "true" : "false") << ",\n";
            file << "      \"nearestNodeId\": " << poi.nearestNodeId << ",\n";
            file << "      \"distanceToNode\": " << poi.distanceToNode << "\n";
            file << "    }";
            
            if (i < allPOIs.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        std::cout << "[POIRegistry] Exported " << allPOIs.size() << " POIs to: " << filepath << std::endl;
        return true;
    }

    void POIRegistry::PrintSummary() const {
        std::cout << "\n========== POI Registry Summary ==========" << std::endl;
        std::cout << "Total POIs: " << allPOIs.size() << std::endl;
        std::cout << "Mapped to NavMesh: " << (isMappedToNavMesh ? "Yes" : "No") << std::endl;
        std::cout << "\nBy Type:" << std::endl;
        
        // Print counts by type
        std::cout << "  CHARGING (Cx):  " << GetPOICountByType(POIType::CHARGING) << std::endl;
        std::cout << "  PICKUP (Px):    " << GetPOICountByType(POIType::PICKUP) << std::endl;
        std::cout << "  DROPOFF (Px):   " << GetPOICountByType(POIType::DROPOFF) << std::endl;
        
        if (!allPOIs.empty()) {
            std::cout << "\nPOI Details:" << std::endl;
            for (const auto& poi : allPOIs) {
                std::cout << "  " << poi.id << " [" << poi.typeName << "] @ (" 
                          << poi.worldCoords.x << ", " << poi.worldCoords.y << ")";
                if (poi.nearestNodeId >= 0) {
                    std::cout << " -> Node " << poi.nearestNodeId 
                              << " (dist: " << poi.distanceToNode << ")";
                }
                std::cout << (poi.isActive ? " [ACTIVE]" : " [INACTIVE]") << std::endl;
            }
        }
        
        std::cout << "==========================================\n" << std::endl;
    }

} // namespace Layer1
} // namespace Backend
