#include "DynamicObstacle.hh"

namespace Backend {
namespace Layer1 {

    DynamicObstacle::DynamicObstacle(Backend::Common::Coordinates anchor, int s)
        : size(s), top_left_anchor(anchor) {}

    std::vector<Backend::Common::Coordinates> DynamicObstacle::GetOccupiedCells() const {
        std::vector<Backend::Common::Coordinates> cells;
        cells.reserve(size * size); 

        for (int x = 0; x < size; ++x) {
            for (int y = 0; y < size; ++y) {
                cells.push_back({
                    top_left_anchor.x + x,
                    top_left_anchor.y + y
                });
            }
        }
        return cells;
    }

} // namespace Layer1
} // namespace Backend