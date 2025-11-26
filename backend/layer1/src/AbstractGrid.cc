#include "AbstractGrid.hh"

namespace Backend {
namespace Layer1 {

    AbstractGrid::AbstractGrid(int w, int h, Backend::Common::Resolution res)
        : width(w), height(h), resolution(res) {}

    std::pair<int, int> AbstractGrid::GetDimensions() const {
        return {width, height};
    }

    bool AbstractGrid::IsWithinBounds(Backend::Common::Coordinates coords) const {
        return coords.x >= 0 && coords.x < width && 
               coords.y >= 0 && coords.y < height;
    }

} // namespace Layer1
} // namespace Backend