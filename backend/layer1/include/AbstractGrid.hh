#ifndef BACKEND_LAYER1_ABSTRACTGRID_HH
#define BACKEND_LAYER1_ABSTRACTGRID_HH

#include <utility> 
#include "Coordinates.hh"
#include "Resolution.hh" 

namespace Backend {
namespace Layer1 {

    class AbstractGrid {
    protected:
        int width;
        int height;
        Backend::Common::Resolution resolution;

    public:
        AbstractGrid(int w, int h, Backend::Common::Resolution res);

        // Virtual Destructor is REQUIRED for inheritance safety
        virtual ~AbstractGrid() = default;

        std::pair<int, int> GetDimensions() const;
        
        bool IsWithinBounds(Backend::Common::Coordinates coords) const;

        // The Pure Virtual Contract
        virtual bool IsAccessible(Backend::Common::Coordinates coords) const = 0;
    };

} // namespace Layer1
} // namespace Backend

#endif