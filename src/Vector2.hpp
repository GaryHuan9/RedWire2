#pragma once

#include "main.hpp"

namespace rw
{

template<class T>
class Vector2
{
public:
	Vector2() : x{}, y{} {}

	Vector2(T x, T y) : x(x), y(y) {}

	explicit Vector2(T value) : x(value), y(value) {}

	T x;
	T y;
};

typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;
typedef Vector2<uint32_t> UInt2;

template<class T>
inline Vector2<T> operator+(Vector2<T> value) { return Vector2<T>(+value.x, +value.y); }

template<class T>
inline Vector2<T> operator-(Vector2<T> value) { return Vector2<T>(-value.x, -value.y); }

template<class T>
inline Vector2<T> operator+(Vector2<T> value, Vector2<T> other) { return Vector2<T>(value.x + other.x, value.y + other.y); }

template<class T>
inline Vector2<T> operator-(Vector2<T> value, Vector2<T> other) { return Vector2<T>(value.x - other.x, value.y - other.y); }

template<class T>
inline Vector2<T> operator*(Vector2<T> value, Vector2<T> other) { return Vector2<T>(value.x * other.x, value.y * other.y); }

template<class T>
inline Vector2<T> operator/(Vector2<T> value, Vector2<T> other) { return Vector2<T>(value.x / other.x, value.y / other.y); }

template<class T>
inline Vector2<T> operator*(Vector2<T> value, T other) { return Vector2<T>(value.x * other, value.y * other); }

template<class T>
inline Vector2<T> operator/(Vector2<T> value, T other) { return Vector2<T>(value.x / other, value.y / other); }

template<class T>
inline Vector2<T> operator+=(Vector2<T>& value, Vector2<T> other) { return Vector2<T>(value.x += other.x, value.y += other.y); }

template<class T>
inline Vector2<T> operator-=(Vector2<T>& value, Vector2<T> other) { return Vector2<T>(value.x -= other.x, value.y -= other.y); }

template<class T>
inline Vector2<T> operator*=(Vector2<T>& value, Vector2<T> other) { return Vector2<T>(value.x *= other.x, value.y *= other.y); }

template<class T>
inline Vector2<T> operator/=(Vector2<T>& value, Vector2<T> other) { return Vector2<T>(value.x /= other.x, value.y /= other.y); }

template<class T>
inline Vector2<T> operator*=(Vector2<T>& value, T other) { return Vector2<T>(value.x *= other, value.y *= other); }

template<class T>
inline Vector2<T> operator/=(Vector2<T>& value, T other) { return Vector2<T>(value.x /= other, value.y /= other); }

template<class T>
inline bool operator==(Vector2<T> value, Vector2<T> other) { return value.x == other.x & value.y == other.y; }

template<class T>
inline bool operator!=(Vector2<T> value, Vector2<T> other) { return value.x != other.x & value.y != other.y; }

template<class T>
inline std::ostream& operator<<(std::ostream& stream, Vector2<T> value) { return stream << '(' << value.x << ", " << value.y << ')'; }

} // rw2

template<class T>
struct std::hash<rw::Vector2<T>>
{
	size_t operator()(rw::Vector2<T> value) const noexcept
	{
		std::hash<T> hasher;
		size_t x = hasher(value.x);
		size_t y = hasher(value.y);
		return x ^ (y + 0x9E3779B9 + (x << 6) + (x >> 2));
	}
};