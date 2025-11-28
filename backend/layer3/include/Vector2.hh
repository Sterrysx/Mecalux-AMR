/**
 * @file Vector2.hh
 * @brief 2D Vector mathematics for physics simulation
 * 
 * Provides a lightweight 2D vector class for Layer 3 physics calculations,
 * including velocity, direction, and force computations.
 */

#ifndef LAYER3_VECTOR2_HH
#define LAYER3_VECTOR2_HH

#include <cmath>
#include <iostream>

namespace Backend {
namespace Layer3 {

/**
 * @brief 2D Vector for physics calculations.
 * 
 * Used for:
 * - Robot velocity vectors
 * - Direction calculations
 * - Force/acceleration vectors
 * - ORCA constraint calculations
 */
struct Vector2 {
    double x;
    double y;

    // =========================================================================
    // CONSTRUCTORS
    // =========================================================================
    
    /**
     * @brief Default constructor (zero vector).
     */
    Vector2() : x(0.0), y(0.0) {}
    
    /**
     * @brief Construct with components.
     */
    Vector2(double x_, double y_) : x(x_), y(y_) {}

    // =========================================================================
    // ARITHMETIC OPERATORS
    // =========================================================================
    
    /**
     * @brief Vector addition.
     */
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    
    /**
     * @brief Vector subtraction.
     */
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    
    /**
     * @brief Scalar multiplication.
     */
    Vector2 operator*(double scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    
    /**
     * @brief Scalar division.
     */
    Vector2 operator/(double scalar) const {
        if (scalar == 0.0) return Vector2(0.0, 0.0);
        return Vector2(x / scalar, y / scalar);
    }
    
    /**
     * @brief Compound addition.
     */
    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    /**
     * @brief Compound subtraction.
     */
    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    /**
     * @brief Compound scalar multiplication.
     */
    Vector2& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    /**
     * @brief Negation.
     */
    Vector2 operator-() const {
        return Vector2(-x, -y);
    }

    // =========================================================================
    // COMPARISON OPERATORS
    // =========================================================================
    
    bool operator==(const Vector2& other) const {
        return (x == other.x) && (y == other.y);
    }
    
    bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    // =========================================================================
    // VECTOR OPERATIONS
    // =========================================================================
    
    /**
     * @brief Calculate vector magnitude (length).
     */
    double Magnitude() const {
        return std::sqrt(x * x + y * y);
    }
    
    /**
     * @brief Calculate squared magnitude (faster, no sqrt).
     */
    double MagnitudeSquared() const {
        return x * x + y * y;
    }
    
    /**
     * @brief Return unit vector (normalized).
     * 
     * @return Normalized vector, or zero vector if magnitude is zero
     */
    Vector2 Normalized() const {
        double mag = Magnitude();
        if (mag < 1e-10) return Vector2(0.0, 0.0);
        return Vector2(x / mag, y / mag);
    }
    
    /**
     * @brief Normalize this vector in place.
     */
    void Normalize() {
        double mag = Magnitude();
        if (mag < 1e-10) {
            x = 0.0;
            y = 0.0;
        } else {
            x /= mag;
            y /= mag;
        }
    }
    
    /**
     * @brief Dot product with another vector.
     */
    double Dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }
    
    /**
     * @brief 2D cross product (returns scalar).
     * 
     * This is the z-component of the 3D cross product.
     * Positive = other is counter-clockwise from this.
     */
    double Cross(const Vector2& other) const {
        return x * other.y - y * other.x;
    }
    
    /**
     * @brief Perpendicular vector (rotated 90° counter-clockwise).
     */
    Vector2 Perpendicular() const {
        return Vector2(-y, x);
    }
    
    /**
     * @brief Distance to another point.
     */
    double DistanceTo(const Vector2& other) const {
        return (*this - other).Magnitude();
    }
    
    /**
     * @brief Squared distance to another point (faster).
     */
    double DistanceSquaredTo(const Vector2& other) const {
        return (*this - other).MagnitudeSquared();
    }
    
    /**
     * @brief Limit magnitude to a maximum value.
     */
    Vector2 LimitMagnitude(double maxMag) const {
        double mag = Magnitude();
        if (mag > maxMag && mag > 1e-10) {
            return (*this) * (maxMag / mag);
        }
        return *this;
    }
    
    /**
     * @brief Linear interpolation to another vector.
     * 
     * @param other Target vector
     * @param t Interpolation factor (0 = this, 1 = other)
     */
    Vector2 Lerp(const Vector2& other, double t) const {
        return *this + (other - *this) * t;
    }
    
    /**
     * @brief Angle of this vector in radians (from positive x-axis).
     */
    double Angle() const {
        return std::atan2(y, x);
    }
    
    /**
     * @brief Angle to another vector in radians.
     */
    double AngleTo(const Vector2& other) const {
        return std::atan2(Cross(other), Dot(other));
    }

    // =========================================================================
    // STATIC FACTORY METHODS
    // =========================================================================
    
    /**
     * @brief Create vector from angle and magnitude.
     */
    static Vector2 FromAngle(double angle, double magnitude = 1.0) {
        return Vector2(std::cos(angle) * magnitude, std::sin(angle) * magnitude);
    }
    
    /**
     * @brief Zero vector.
     */
    static Vector2 Zero() {
        return Vector2(0.0, 0.0);
    }
    
    /**
     * @brief Unit vector pointing right (+X).
     */
    static Vector2 Right() {
        return Vector2(1.0, 0.0);
    }
    
    /**
     * @brief Unit vector pointing up (+Y).
     */
    static Vector2 Up() {
        return Vector2(0.0, 1.0);
    }

    // =========================================================================
    // STREAM OUTPUT
    // =========================================================================
    
    friend std::ostream& operator<<(std::ostream& os, const Vector2& v) {
        os << "(" << v.x << ", " << v.y << ")";
        return os;
    }
};

/**
 * @brief Scalar * Vector multiplication (commutative support).
 */
inline Vector2 operator*(double scalar, const Vector2& v) {
    return v * scalar;
}

} // namespace Layer3
} // namespace Backend

#endif // LAYER3_VECTOR2_HH
