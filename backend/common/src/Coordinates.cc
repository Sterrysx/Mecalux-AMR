#include "common/Coordinates.hh"
#include <cmath> // For std::sqrt, std::pow

namespace Backend {
namespace Common {

    bool Coordinates::operator==(const Coordinates& other) const {
        return x == other.x && y == other.y;
    }

    bool Coordinates::operator!=(const Coordinates& other) const {
        return !(*this == other);
    }

    float Coordinates::DistanceTo(const Coordinates& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    }

} // namespace Common
} // namespace Backend