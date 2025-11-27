#ifndef BACKEND_COMMON_RESOLUTION_HH
#define BACKEND_COMMON_RESOLUTION_HH

namespace Backend {
namespace Common {

    /**
     * @brief Defines the spatial granularity of the system.
     * Shared between Mapping (Layer 1) and Physics (Layer 3).
     * 
     * Robot occupancy in each resolution:
     * - DECIMETERS:  Robot (60cm) = 6x6 pixels, node = 5x5 with center
     * - CENTIMETERS: Robot (60cm) = 60x60 pixels
     * - MILLIMETERS: Robot (60cm) = 600x600 pixels
     */
    enum class Resolution {
        DECIMETERS,   // 1 pixel = 0.1m (10cm)
        CENTIMETERS,  // 1 pixel = 0.01m (1cm)
        MILLIMETERS   // 1 pixel = 0.001m (1mm)
    };

    /**
     * @brief Helper to get the conversion factor to Meters.
     * Example: If res is CENTIMETERS, returns 0.01.
     */
    double GetConversionFactorToMeters(Resolution res);
    
    /**
     * @brief Get the robot footprint size in pixels for a given resolution.
     * Robot physical size is 60cm x 60cm.
     */
    int GetRobotFootprintPixels(Resolution res);
    
    /**
     * @brief Get a human-readable name for the resolution.
     */
    const char* GetResolutionName(Resolution res);

} // namespace Common
} // namespace Backend

#endif // BACKEND_COMMON_RESOLUTION_HH