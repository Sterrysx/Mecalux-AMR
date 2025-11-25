#ifndef BACKEND_COMMON_RESOLUTION_HH
#define BACKEND_COMMON_RESOLUTION_HH

namespace Backend {
namespace Common {

    /**
     * @brief Defines the spatial granularity of the system.
     * Shared between Mapping (Layer 1) and Physics (Layer 3).
     */
    enum class Resolution {
        METERS,
        DECIMETERS,
        CENTIMETERS,
        MILLIMETERS
    };

    /**
     * @brief Helper to get the conversion factor to Meters.
     * Example: If res is CENTIMETERS, returns 0.01.
     */
    double GetConversionFactorToMeters(Resolution res);

} // namespace Common
} // namespace Backend

#endif // BACKEND_COMMON_RESOLUTION_HH