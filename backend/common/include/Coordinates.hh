#ifndef BACKEND_COMMON_COORDINATES_HH
#define BACKEND_COMMON_COORDINATES_HH

namespace Backend {
namespace Common {

    struct Coordinates {
        int x;
        int y;

        // Equality operator is crucial for pathfinding (Set/Map keys)
        bool operator==(const Coordinates& other) const;
        bool operator!=(const Coordinates& other) const;

        // Euclidean distance helper
        float DistanceTo(const Coordinates& other) const;
    };

} // namespace Common
} // namespace Backend

#endif // BACKEND_COMMON_COORDINATES_HH