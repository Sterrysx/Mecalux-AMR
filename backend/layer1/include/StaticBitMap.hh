#ifndef BACKEND_LAYER1_STATICBITMAP_HH
#define BACKEND_LAYER1_STATICBITMAP_HH

#include <vector>
#include <string>
#include <utility>
#include "AbstractGrid.hh"

namespace Backend {
namespace Layer1 {

    class StaticBitMap : public AbstractGrid {
    private:
        // Flattened 2D grid: index = y * width + x
        std::vector<bool> gridData;

    public:
        // Default constructor for manual sizing
        StaticBitMap(int w, int h, Backend::Common::Resolution res);

        // Implement the contract
        bool IsAccessible(Backend::Common::Coordinates coords) const override;

        // Load map from file - reads dimensions from first line, then parses grid
        // '.' = walkable, '#' = obstacle
        void LoadFromFile(const std::string& filepath);
        
        // Used by DynamicBitMap to clone data
        const std::vector<bool>& GetRawData() const;
        
        // Static factory to create from file with auto-detected dimensions
        static StaticBitMap CreateFromFile(const std::string& filepath, 
                                           Backend::Common::Resolution res);
        
        // Static utility to get file dimensions before loading
        static std::pair<int, int> GetFileDimensions(const std::string& filepath);
    };

} // namespace Layer1
} // namespace Backend

#endif