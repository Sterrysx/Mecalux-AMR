#include "DynamicBitMap.hh"

namespace Backend {
namespace Layer1 {

    DynamicBitMap::DynamicBitMap(const StaticBitMap& source)
        : AbstractGrid(source.GetDimensions().first, source.GetDimensions().second, Backend::Common::Resolution::METERS) {
        
        activeGrid = source.GetRawData();
    }

    bool DynamicBitMap::IsAccessible(Backend::Common::Coordinates coords) const {
        // LOCK READ
        std::lock_guard<std::mutex> lock(mapMutex);

        if (!IsWithinBounds(coords)) return false;
        return activeGrid[coords.y * width + coords.x];
    }

    void DynamicBitMap::Update(const std::vector<DynamicObstacle>& obstacles, const StaticBitMap& source) {
        // LOCK WRITE
        std::lock_guard<std::mutex> lock(mapMutex);

        // 1. Wipe Canvas (Reset to Static)
        activeGrid = source.GetRawData();

        // 2. Paint Obstacles
        for (const auto& obs : obstacles) {
            for (const auto& cell : obs.GetOccupiedCells()) {
                if (IsWithinBounds(cell)) {
                    activeGrid[cell.y * width + cell.x] = false;
                }
            }
        }
    }

} // namespace Layer1
} // namespace Backend