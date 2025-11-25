#ifndef BACKEND_LAYER1_NAVMESHGENERATOR_HH
#define BACKEND_LAYER1_NAVMESHGENERATOR_HH

#include "StaticBitMap.hh"
#include "NavMesh.hh"

namespace Backend {
namespace Layer1 {

    class NavMeshGenerator {
    public:
        /**
         * @brief The main factory method.
         * Reads the static grid, performs Recast/Decomposition, 
         * and populates the NavMesh object.
         * * @param map The input static bitmap (pixels)
         * @param mesh The output graph object to populate
         */
        void ComputeRecast(const StaticBitMap& map, NavMesh& mesh);
    };

} // namespace Layer1
} // namespace Backend

#endif