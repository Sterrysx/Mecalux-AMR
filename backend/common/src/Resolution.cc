#include "Resolution.hh"

namespace Backend {
namespace Common {

    double GetConversionFactorToMeters(Resolution res) {
        switch (res) {
            case Resolution::DECIMETERS:  return 0.1;
            case Resolution::CENTIMETERS: return 0.01;
            case Resolution::MILLIMETERS: return 0.001;
            default:                      return 0.1; // Default to decimeters
        }
    }
    
    int GetRobotFootprintPixels(Resolution res) {
        // Robot is 60cm x 60cm = 0.6m
        // We use 5x5 for decimeters (50cm, slightly smaller for safety margin)
        switch (res) {
            case Resolution::DECIMETERS:  return 5;   // 5 pixels = 50cm (with center)
            case Resolution::CENTIMETERS: return 60;  // 60 pixels = 60cm
            case Resolution::MILLIMETERS: return 600; // 600 pixels = 60cm
            default:                      return 5;
        }
    }
    
    const char* GetResolutionName(Resolution res) {
        switch (res) {
            case Resolution::DECIMETERS:  return "DECIMETERS";
            case Resolution::CENTIMETERS: return "CENTIMETERS";
            case Resolution::MILLIMETERS: return "MILLIMETERS";
            default:                      return "UNKNOWN";
        }
    }

} // namespace Common
} // namespace Backend