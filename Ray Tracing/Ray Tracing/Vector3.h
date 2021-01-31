#pragma once

#include <array>
#include <fstream>

class Vector3
{
public:
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };

        std::array<float, 3> v;
    };

    Vector3() = default;
    Vector3(float desiredX, float desiredY, float desiredZ)
    {
        x = desiredX;
        y = desiredY;
        z = desiredZ;
    }

    inline float& operator[](size_t index)
    {
        return v[index];
    }

    inline float operator[](size_t index) const
    {
        return v[index];
    }

    inline const Vector3& operator+()
    {
        return *this;
    }

    inline Vector3 operator-() const
    {
        return Vector3(-x, -y, -z);
    }

    inline Vector3& operator+=(const Vector3& param);
    inline Vector3& operator-=(const Vector3& param);
    inline Vector3& operator*=(const Vector3& param);
    inline Vector3& operator/=(const Vector3& param);
    inline Vector3& operator*=(const float param);
    inline Vector3& operator/=(const float param);

    inline float Length() const
    {
        return std::sqrt(SquaredLength());
    }

    inline float SquaredLength() const
    {
        return x * x + y * y + z * z;
    }

    inline void Normalize()
    {
        float k = 1.0f / Length();
        x *= k;
        y *= k;
        z *= k;
    }

};

inline std::istream& operator>>(std::istream& is, Vector3& v)
{
    is >> v.x >> v.y >> v.z;
    return is;
}

inline std::ostream& operator<<(std::ostream& os, Vector3& v)
{
    os << v.x << v.y << v.z;
    return os;
}

inline Vector3 operator+(const Vector3& v1, const Vector3& v2)
{
    return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline Vector3 operator-(const Vector3& v1, const Vector3& v2)
{
    return Vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline Vector3 operator*(const Vector3& v1, const Vector3& v2)
{
    return Vector3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

inline Vector3 operator/(const Vector3& v1, const Vector3& v2)
{
    return Vector3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

inline Vector3 operator*(float t, const Vector3& v)
{
    return Vector3(t * v.x, t * v.y, t * v.z);
}

inline Vector3 operator/(Vector3 v, float t)
{
    return Vector3(v.x / t, v.y / t, v.z / t);
}

inline Vector3 operator*(const Vector3& v, float t)
{
    return Vector3(t * v.x, t * v.y, t * v.z);
}

inline float DotProduct(const Vector3& v1, const Vector3& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2)
{
    return Vector3((v1.y * v2.z - v1.z * v2.y),
                   (v1.z * v2.x - v1.x * v2.z),
                   (v1.x * v2.y - v1.y * v2.x));
}

inline Vector3& Vector3::operator+=(const Vector3& v)
{
    this->x += v.x;
    this->y += v.y;
    this->z += v.z;
    return *this;
}

inline Vector3& Vector3::operator*=(const Vector3& v)
{
    this->x *= v.x;
    this->y *= v.y;
    this->z *= v.z;
    return *this;
}

inline Vector3& Vector3::operator/=(const Vector3& v)
{
    this->x /= v.x;
    this->y /= v.y;
    this->z /= v.z;
    return *this;
}

inline Vector3& Vector3::operator-=(const Vector3& v)
{
    this->x -= v.x;
    this->y -= v.y;
    this->z -= v.z;
    return *this;
}

inline Vector3& Vector3::operator*=(const float t)
{
    x *= t;
    y *= t;
    z *= t;
    return *this;
}

inline Vector3& Vector3::operator/=(const float t)
{
    float k = 1.0f / t;

    x *= k;
    y *= k;
    z *= k;
    return *this;
}

inline Vector3 GetNormalized(Vector3 v)
{
    return v / v.Length();
}
