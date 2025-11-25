#include "DynamicObstacleManager.hh"
#include <cstdlib> // rand
#include <ctime>   // time

namespace Backend {
namespace Layer1 {

    void DynamicObstacleManager::SpawnRandomObstacle(int mapWidth, int mapHeight) {
        int size = (std::rand() % 3) + 1; // Random size 1-3
        
        int maxX = mapWidth - size;
        int maxY = mapHeight - size;

        if (maxX <= 0 || maxY <= 0) return; 

        int x = std::rand() % maxX;
        int y = std::rand() % maxY;

        active_obstacles.emplace_back(Backend::Common::Coordinates{x, y}, size);
    }

    const std::vector<DynamicObstacle>& DynamicObstacleManager::GetActiveObstacles() const {
        return active_obstacles;
    }

    void DynamicObstacleManager::Clear() {
        active_obstacles.clear();
    }

} // namespace Layer1
} // namespace Backend