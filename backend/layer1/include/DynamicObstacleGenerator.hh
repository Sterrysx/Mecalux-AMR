#ifndef BACKEND_LAYER1_DYNAMICOBSTACLEMANAGER_HH
#define BACKEND_LAYER1_DYNAMICOBSTACLEMANAGER_HH

#include <vector>
#include "DynamicObstacle.hh"

namespace Backend {
namespace Layer1 {

    class DynamicObstacleManager {
    private:
        std::vector<DynamicObstacle> active_obstacles;

    public:
        // Spawns a new obstacle within the given map bounds
        void SpawnRandomObstacle(int mapWidth, int mapHeight);
        
        // Spawns an obstacle at a specific location with given size
        void SpawnObstacleAt(Backend::Common::Coordinates coords, int size);

        const std::vector<DynamicObstacle>& GetActiveObstacles() const;
        
        void Clear();
    };

} // namespace Layer1
} // namespace Backend

#endif