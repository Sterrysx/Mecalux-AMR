#ifndef BACKEND_LAYER1_DYNAMICBITMAP_HH
#define BACKEND_LAYER1_DYNAMICBITMAP_HH

#include <vector>
#include <mutex>
#include "AbstractGrid.hh"
#include "StaticBitMap.hh"
#include "DynamicObstacle.hh"

namespace Backend {
namespace Layer1 {

    class DynamicBitMap : public AbstractGrid {
    private:
        std::vector<bool> activeGrid;
        
        // Critical for Layer 3 (Reading) vs Layer 1 (Writing) safety
        mutable std::mutex mapMutex; 

    public:
        // Constructor copies the static map initially
        explicit DynamicBitMap(const StaticBitMap& source);

        bool IsAccessible(Backend::Common::Coordinates coords) const override;

        // The Update Loop
        // We use the Static Map to "Reset" the canvas, then paint obstacles
        void Update(const std::vector<DynamicObstacle>& obstacles, const StaticBitMap& source);
    };

} // namespace Layer1
} // namespace Backend

#endif