#ifndef BACKEND_LAYER1_INFLATEDBITMAP_HH
#define BACKEND_LAYER1_INFLATEDBITMAP_HH

#include <vector>
#include "AbstractGrid.hh"
#include "StaticBitMap.hh"

namespace Backend {
namespace Layer1 {

    /**
     * @brief Configuration Space map with inflated obstacles.
     * 
     * This class creates a "safety geometry" view of the map where every obstacle
     * (wall) is expanded by the robot's radius. This creates a "no-go zone" around
     * obstacles that guarantees:
     * 
     * 1. If a center pixel is accessible, the entire robot body will fit
     * 2. Narrow corridors that look walkable but are too tight are automatically closed
     * 3. The pathfinder can perform simple point-based checks instead of area calculations
     * 
     * The inflation is computed once at construction time using a Minkowski Sum approach:
     * For each obstacle cell, mark all cells within robot_radius as inaccessible.
     * 
     * This is the map that should be used by NavMeshGenerator to build the navigation graph.
     */
    class InflatedBitMap : public AbstractGrid {
    private:
        // Flattened 2D grid: index = y * width + x
        // false = inaccessible (obstacle or within robot radius of obstacle)
        // true = accessible (safe for robot center)
        std::vector<bool> inflatedGrid;
        
        // The inflation radius in pixels (derived from robot physical radius)
        int inflationRadiusPixels;
        
        // Store the original static map reference for comparisons
        const StaticBitMap* sourceMap;

    public:
        /**
         * @brief Construct an InflatedBitMap from a StaticBitMap.
         * 
         * @param source The original static bitmap with raw obstacle data
         * @param robotRadiusMeters The physical robot radius in meters
         *                          (will be converted to pixels based on resolution)
         * 
         * The constructor performs the inflation operation:
         * 1. Copy dimensions and resolution from source
         * 2. Calculate inflation radius in pixels
         * 3. For each obstacle cell in source, mark surrounding cells as inaccessible
         */
        InflatedBitMap(const StaticBitMap& source, float robotRadiusMeters);

        /**
         * @brief Check if a coordinate is accessible in the inflated map.
         * 
         * A cell is accessible only if:
         * 1. It is within bounds
         * 2. It is not an obstacle in the original map
         * 3. It is not within the inflation radius of any obstacle
         * 
         * @param coords The coordinates to check
         * @return true if accessible (safe for robot center), false otherwise
         */
        bool IsAccessible(Backend::Common::Coordinates coords) const override;

        /**
         * @brief Get the inflation radius in pixels.
         * 
         * @return The number of pixels used for obstacle inflation
         */
        int GetInflationRadiusPixels() const;

        /**
         * @brief Get the raw inflated grid data.
         * 
         * Useful for debugging, visualization, or for DynamicInflatedBitMap
         * if we need to combine with dynamic obstacles.
         * 
         * @return Reference to the internal grid data
         */
        const std::vector<bool>& GetRawData() const;

        /**
         * @brief Get a reference to the original source map.
         * 
         * @return Pointer to the source StaticBitMap
         */
        const StaticBitMap* GetSourceMap() const;

        /**
         * @brief Export the inflated map to a file for visualization.
         * 
         * Useful for debugging to see which areas have been closed off.
         * Format matches StaticBitMap: '.' = accessible, '#' = obstacle
         * 
         * @param filepath Path to the output file
         */
        void ExportToFile(const std::string& filepath) const;

        /**
         * @brief Get statistics about the inflation effect.
         * 
         * @param originalWalkable Output: cells walkable in original map
         * @param inflatedWalkable Output: cells walkable after inflation
         * @param closedCells Output: cells closed by inflation
         */
        void GetInflationStats(int& originalWalkable, int& inflatedWalkable, int& closedCells) const;
    };

} // namespace Layer1
} // namespace Backend

#endif // BACKEND_LAYER1_INFLATEDBITMAP_HH
