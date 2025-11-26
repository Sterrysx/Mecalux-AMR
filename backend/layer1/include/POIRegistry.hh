#ifndef BACKEND_LAYER1_POIREGISTRY_HH
#define BACKEND_LAYER1_POIREGISTRY_HH

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include "Coordinates.hh"
#include "NavMesh.hh"
#include "AbstractGrid.hh"

namespace Backend {
namespace Layer1 {

    /**
     * @brief Types of Points of Interest supported by the system.
     * 
     * Two main categories:
     * - CHARGING: Where robots recharge batteries (C0, C1, C2...)
     * - Packet zones (P0, P1, P2...) with two sub-types:
     *   - PICKUP:  Robot arrives empty, leaves with package
     *   - DROPOFF: Robot arrives with package, leaves empty
     * 
     * Note: PICKUP and DROPOFF are kept distinct for Layer 2 (Fleet Manager)
     * which needs to know the semantic difference for task assignment.
     * Visualizers may group them as "Packet Zones" but the backend tracks the type.
     */
    enum class POIType {
        CHARGING,       // Charging stations for robot batteries (C0, C1, C2...)
        PICKUP,         // Pickup zones - robot collects packages (P0, P1...)
        DROPOFF         // Dropoff zones - robot delivers packages (P2, P3...)
    };

    /**
     * @brief Represents a single Point of Interest in the warehouse.
     */
    struct PointOfInterest {
        std::string id;                         // Unique identifier (e.g., "C0", "P1")
        POIType type;                           // Type of POI (CHARGING, PICKUP, or DROPOFF)
        std::string typeName;                   // String name ("CHARGING", "PICKUP", "DROPOFF")
        Backend::Common::Coordinates worldCoords; // Physical world coordinates (pixels)
        int nearestNodeId;                      // Cached NavMesh node ID (-1 if not mapped)
        float distanceToNode;                   // Distance from worldCoords to node center
        bool isActive;                          // Whether this POI is currently operational
        
        // Additional metadata
        std::map<std::string, std::string> metadata; // Custom key-value pairs (e.g., capacity, priority)
    };

    /**
     * @brief Registry for managing Points of Interest in the warehouse.
     * 
     * This class provides a semantic overlay on top of the NavMesh, allowing
     * Layer 2 (Planner) to query for specific locations by type rather than
     * scanning coordinates.
     * 
     * POI Types:
     * - CHARGING zones: Where robots charge (IDs: C0, C1, C2...)
     * - PICKUP zones: Where robots collect packages (IDs: P0, P1...)
     * - DROPOFF zones: Where robots deliver packages (IDs: P2, P3...)
     * 
     * Key Features:
     * - Load POI definitions from JSON configuration
     * - Automatically map POIs to nearest valid NavMesh nodes
     * - Provide O(1) lookup by type: GetNodesByType(POIType::CHARGING)
     * 
     * Usage:
     *   POIRegistry registry;
     *   registry.LoadFromJSON("poi_config.json");
     *   registry.MapToNavMesh(navMesh);
     *   auto chargerNodes = registry.GetNodesByType(POIType::CHARGING);
     *   auto pickupNodes = registry.GetNodesByType(POIType::PICKUP);
     *   auto dropoffNodes = registry.GetNodesByType(POIType::DROPOFF);
     */
    class POIRegistry {
    private:
        // All registered POIs
        std::vector<PointOfInterest> allPOIs;
        
        // Fast lookup by type: type -> list of POI indices
        std::unordered_map<POIType, std::vector<int>> poiByType;
        
        // Fast lookup by type name (for custom types)
        std::unordered_map<std::string, std::vector<int>> poiByTypeName;
        
        // Fast lookup by NavMesh node ID: nodeId -> list of POI indices
        std::unordered_map<int, std::vector<int>> poiByNodeId;
        
        // Fast lookup by POI ID
        std::unordered_map<std::string, int> poiById;
        
        // Whether POIs have been mapped to NavMesh
        bool isMappedToNavMesh;

    public:
        /**
         * @brief Default constructor.
         */
        POIRegistry();

        // =========================================================================
        // LOADING
        // =========================================================================

        /**
         * @brief Load POI definitions from a JSON configuration file.
         * 
         * Expected JSON format:
         * {
         *   "poi": [
         *     {
         *       "id": "CHARGER_01",
         *       "type": "CHARGER",
         *       "x": 100,
         *       "y": 50,
         *       "active": true,
         *       "metadata": { "power_kw": "5.0" }
         *     },
         *     ...
         *   ]
         * }
         * 
         * @param filepath Path to the JSON configuration file
         * @return true if loading succeeded, false otherwise
         */
        bool LoadFromJSON(const std::string& filepath);

        /**
         * @brief Add a POI programmatically.
         * 
         * @param id Unique identifier
         * @param type POI type
         * @param coords World coordinates
         * @param active Whether POI is operational
         * @return Index of the added POI, or -1 if ID already exists
         */
        int AddPOI(const std::string& id, POIType type, 
                   Backend::Common::Coordinates coords, bool active = true);

        /**
         * @brief Add a custom-type POI.
         * 
         * @param id Unique identifier
         * @param typeName Custom type name
         * @param coords World coordinates
         * @param active Whether POI is operational
         * @return Index of the added POI, or -1 if ID already exists
         */
        int AddCustomPOI(const std::string& id, const std::string& typeName,
                         Backend::Common::Coordinates coords, bool active = true);

        // =========================================================================
        // NAVMESH MAPPING & SAFETY VALIDATION
        // =========================================================================

        /**
         * @brief Validate POIs against safety map and map to NavMesh nodes.
         * 
         * THIS IS THE RECOMMENDED METHOD - validates that each POI coordinate
         * is in a safe zone where the robot can physically fit.
         * 
         * The safetyMap should be an InflatedBitMap. A POI is safe if:
         * - safetyMap.IsAccessible(poi.coords) == true
         * 
         * POIs that fail validation are automatically disabled (isActive = false)
         * and a CRITICAL WARNING is printed.
         * 
         * @param mesh The NavMesh to map POIs to
         * @param safetyMap The inflated bitmap that encodes robot clearance
         * @param maxDistance Maximum allowed distance from POI to node (0 = no limit)
         * @return Number of successfully validated and mapped POIs
         */
        int ValidateAndMapToNavMesh(const NavMesh& mesh, const AbstractGrid& safetyMap, 
                                     float maxDistance = 0.0f);

        /**
         * @brief Map all POIs to their nearest NavMesh nodes (NO SAFETY CHECK).
         * 
         * WARNING: This method does NOT validate POI coordinates against the
         * Configuration Space. Use ValidateAndMapToNavMesh() instead.
         * 
         * @param mesh The NavMesh to map POIs to
         * @param maxDistance Maximum allowed distance from POI to node (0 = no limit)
         * @return Number of successfully mapped POIs
         */
        int MapToNavMesh(const NavMesh& mesh, float maxDistance = 0.0f);

        /**
         * @brief Check if POIs have been mapped to NavMesh.
         */
        bool IsMapped() const;

        // =========================================================================
        // QUERIES BY TYPE
        // =========================================================================

        /**
         * @brief Get all NavMesh node IDs for a specific POI type.
         * 
         * This is the main query method for Layer 2.
         * Example: GetNodesByType(POIType::CHARGER) returns all charging station nodes.
         * 
         * @param type The POI type to query
         * @return Vector of NavMesh node IDs (empty if none found or not mapped)
         */
        std::vector<int> GetNodesByType(POIType type) const;

        /**
         * @brief Get all NavMesh node IDs for a custom POI type name.
         * 
         * @param typeName The custom type name to query
         * @return Vector of NavMesh node IDs
         */
        std::vector<int> GetNodesByTypeName(const std::string& typeName) const;

        /**
         * @brief Get all POIs of a specific type.
         * 
         * @param type The POI type to query
         * @return Vector of POI references
         */
        std::vector<const PointOfInterest*> GetPOIsByType(POIType type) const;

        // =========================================================================
        // QUERIES BY NODE
        // =========================================================================

        /**
         * @brief Get all POIs at a specific NavMesh node.
         * 
         * Multiple POIs can share the same node (e.g., two chargers at the same spot).
         * 
         * @param nodeId The NavMesh node ID
         * @return Vector of POI references at that node
         */
        std::vector<const PointOfInterest*> GetPOIsAtNode(int nodeId) const;

        /**
         * @brief Check if a node has any POIs of a specific type.
         * 
         * @param nodeId The NavMesh node ID
         * @param type The POI type to check for
         * @return true if the node has at least one POI of the specified type
         */
        bool NodeHasPOIType(int nodeId, POIType type) const;

        // =========================================================================
        // QUERIES BY ID
        // =========================================================================

        /**
         * @brief Get a POI by its unique ID.
         * 
         * @param id The POI identifier
         * @return Pointer to the POI, or nullptr if not found
         */
        const PointOfInterest* GetPOIById(const std::string& id) const;

        /**
         * @brief Get the NavMesh node ID for a specific POI.
         * 
         * @param poiId The POI identifier
         * @return The node ID, or -1 if not found or not mapped
         */
        int GetNodeForPOI(const std::string& poiId) const;

        // =========================================================================
        // STATE MANAGEMENT
        // =========================================================================

        /**
         * @brief Set the active status of a POI.
         * 
         * This allows runtime enabling/disabling of POIs (e.g., charger out of service).
         * 
         * @param id The POI identifier
         * @param active New active status
         * @return true if POI was found and updated
         */
        bool SetPOIActive(const std::string& id, bool active);

        /**
         * @brief Get only active POI nodes of a specific type.
         * 
         * @param type The POI type
         * @return Vector of node IDs for active POIs only
         */
        std::vector<int> GetActiveNodesByType(POIType type) const;

        // =========================================================================
        // UTILITIES
        // =========================================================================

        /**
         * @brief Get the total number of registered POIs.
         */
        size_t GetPOICount() const;

        /**
         * @brief Get count of POIs by type.
         */
        size_t GetPOICountByType(POIType type) const;

        /**
         * @brief Clear all POIs and mappings.
         */
        void Clear();

        /**
         * @brief Export POI registry to JSON file.
         * 
         * @param filepath Output file path
         * @return true if export succeeded
         */
        bool ExportToJSON(const std::string& filepath) const;

        /**
         * @brief Print POI summary to console.
         */
        void PrintSummary() const;

        // =========================================================================
        // TYPE CONVERSION UTILITIES
        // =========================================================================

        /**
         * @brief Convert POIType enum to string.
         */
        static std::string TypeToString(POIType type);

        /**
         * @brief Convert string to POIType enum.
         * Returns CUSTOM if string doesn't match any known type.
         */
        static POIType StringToType(const std::string& typeStr);
    };

} // namespace Layer1
} // namespace Backend

#endif // BACKEND_LAYER1_POIREGISTRY_HH
