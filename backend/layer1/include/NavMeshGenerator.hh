#ifndef BACKEND_LAYER1_NAVMESHGENERATOR_HH
#define BACKEND_LAYER1_NAVMESHGENERATOR_HH

#include "AbstractGrid.hh"
#include "NavMesh.hh"

namespace Backend {
namespace Layer1 {

    /**
     * @brief Generates a NavMesh graph from a grid-based map.
     * 
     * This generator uses uniform tiling to create a navigation graph
     * suitable for path planning. It can accept any AbstractGrid derivative:
     * - StaticBitMap: Raw obstacle map
     * - InflatedBitMap: Configuration space with robot radius safety margin
     * - DynamicBitMap: Real-time updated obstacles
     * 
     * For safety-critical pathfinding, use InflatedBitMap to ensure the
     * robot body will physically fit through all generated paths.
     */
    class NavMeshGenerator {
    public:
        /**
         * @brief The main factory method.
         * Reads the grid, performs uniform tiling, and populates the NavMesh object.
         * 
         * @param map The input grid (any AbstractGrid derivative).
         *            Use InflatedBitMap for configuration space pathfinding.
         * @param mesh The output graph object to populate
         */
        void ComputeRecast(const AbstractGrid& map, NavMesh& mesh);
    };

} // namespace Layer1
} // namespace Backend

#endif