#include "Resolution.hh"

namespace Backend {
namespace Common {

    double GetConversionFactorToMeters(Resolution res) {
        switch (res) {
            case Resolution::METERS:      return 1.0;
            case Resolution::DECIMETERS:  return 0.1;
            case Resolution::CENTIMETERS: return 0.01;
            case Resolution::MILLIMETERS: return 0.001;
            default:                      return 1.0;
        }
    }

} // namespace Common
} // namespace Backend