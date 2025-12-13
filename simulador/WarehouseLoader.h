#ifndef WAREHOUSELOADER_H
#define WAREHOUSELOADER_H

#include <QString>
#include <vector>
#include <map>
#include <tuple>
#include "glm/glm.hpp"

struct WarehouseObject {
    int modelIndex;           // Which model from the models array to use
    glm::vec3 center;         // Center position (x, y, z)
    glm::vec3 dimensions;     // Dimensions (width, height, depth)
    float rotation;           // Rotation angle in degrees (around Y axis)
    
    WarehouseObject() : modelIndex(0), center(0.0f), dimensions(1.0f), rotation(0.0f) {}
};

struct RobotData {
    float x;                  // X position
    float y;                  // Y position (Z in world coordinates)
    float angle;              // Rotation angle in degrees
    
    RobotData() : x(0.0f), y(0.0f), angle(0.0f) {}
    RobotData(float x_, float y_, float angle_) : x(x_), y(y_), angle(angle_) {}
};

struct PickingZone {
    std::string type;         // "PICKUP" or "DROPOFF"
    glm::vec3 center;         // Center position (x, y, z)
    glm::vec3 dimensions;     // Dimensions (width, height, depth)
    float rotation;           // Rotation angle in degrees
    std::string description;  // Zone description
    
    PickingZone() : type("PICKUP"), center(0.0f), dimensions(1.0f), rotation(0.0f), description("") {}
};

class WarehouseLoader {
public:
    WarehouseLoader();
    ~WarehouseLoader();
    
    // Load warehouse configuration from JSON file
    bool loadFromJSON(const QString& filename);
    
    // Get all static objects in the warehouse
    const std::vector<WarehouseObject>& getObjects() const { return objects; }
    
    // Get all dynamic robots in the warehouse
    const std::map<int, std::tuple<float, float, float, bool>>& getRobots() const { return robots; }
    
    // Get floor dimensions
    glm::vec2 getFloorSize() const { return floorSize; }
    
    // Get all picking zones
    const std::vector<PickingZone>& getPickingZones() const { return pickingZones; }
    
    // Clear all data
    void clear();
    
private:
    std::vector<WarehouseObject> objects;  // Static warehouse objects
    std::map<int, std::tuple<float, float, float, bool>> robots;  // Dynamic robots (id -> (x, y, angle, hasBox))
    std::vector<PickingZone> pickingZones;  // Picking zones
    glm::vec2 floorSize;  // Floor dimensions (width, depth)
    int nextRobotID;
};

#endif // WAREHOUSELOADER_H
