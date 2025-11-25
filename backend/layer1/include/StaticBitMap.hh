#ifndef BACKEND_LAYER1_STATICBITMAP_HH
#define BACKEND_LAYER1_STATICBITMAP_HH

#include <vector>
#include <string>
#include "layer1/AbstractGrid.hh"

namespace Backend {
namespace Layer1 {

    class StaticBitMap : public AbstractGrid {
    private:
        // Flattened 2D grid: index = y * width + x
        std::vector<bool> gridData;

    public:
        StaticBitMap(int w, int h, Backend::Common::Resolution res);

        // Implement the contract
        bool IsAccessible(Backend::Common::Coordinates coords) const override;

        void LoadFromFile();
        
        // Used by DynamicBitMap to clone data
        const std::vector<bool>& GetRawData() const;
    };

} // namespace Layer1
} // namespace Backend

#endif