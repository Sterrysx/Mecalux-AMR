#include "layer1/StaticBitMap.hh"
#include <iostream> // For mockup logging

namespace Backend {
namespace Layer1 {

    StaticBitMap::StaticBitMap(int w, int h, Backend::Common::Resolution res)
        : AbstractGrid(w, h, res) {
        // Initialize all true (walkable)
        gridData.resize(w * h, true);
    }

    bool StaticBitMap::IsAccessible(Backend::Common::Coordinates coords) const {
        if (!IsWithinBounds(coords)) return false;
        return gridData[coords.y * width + coords.x];
    }

    void StaticBitMap::LoadFromFile() {
        std::cout << "[StaticBitMap] Loading map layout..." << std::endl;
        // Mockup: Create a wall at (5,5)
        if (width > 5 && height > 5) {
            gridData[5 * width + 5] = false; 
        }
    }

    const std::vector<bool>& StaticBitMap::GetRawData() const {
        return gridData;
    }

} // namespace Layer1
} // namespace Backend