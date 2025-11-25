#ifndef BACKEND_LAYER1_DYNAMICOBSTACLE_HH
#define BACKEND_LAYER1_DYNAMICOBSTACLE_HH

#include <vector>
#include "Coordinates.hh" // Assuming ../common/include is in Makefile path

namespace Backend {
namespace Layer1 {

    class DynamicObstacle {
    private:
        int size; 
        Backend::Common::Coordinates top_left_anchor;

    public:
        DynamicObstacle(Backend::Common::Coordinates anchor, int s);

        std::vector<Backend::Common::Coordinates> GetOccupiedCells() const;
    };

} // namespace Layer1
} // namespace Backend

#endif