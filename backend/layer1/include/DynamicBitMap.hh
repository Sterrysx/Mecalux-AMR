#ifndef BACKEND_LAYER1_DYNAMICBITMAP_HH
#define BACKEND_LAYER1_DYNAMICBITMAP_HH

#include <vector>
#include <mutex>
#include <set> // For set<DynamicObstacle>
#include "layer1/AbstractGrid.hh"
#include "layer1/StaticBitMap.hh"

// Forward Declaration to avoid circular include issues if they arise later
namespace Backend { namespace Layer1 { namespace Core { class DynamicObstacle; } } }

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
        void Update(const std::vector<Core::DynamicObstacle>& obstacles, const StaticBitMap& source);
    };

} // namespace Layer1
} // namespace Backend

#endif