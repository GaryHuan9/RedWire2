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

	template<class U>
	Vector2(U x, U y) : x(static_cast<T>(x)), y(static_cast<T>(y)) {}

	explicit Vector2(T value) : Vector2(value, value) {}

	template<class U>
	explicit Vector2(U value) : Vector2(value.x, value.y) {}

	static Int2 ceil(Vector2 value) { return Int2(std::ceil(value.x), std::ceil(value.y)); }

	static Int2 floor(Vector2 value) { return Int2(std::floor(value.x), std::floor(value.y)); }

	T x;
	T y;
};

template<class T>
inline Vector2<T> operator+(Vector2<T> value) { return Vector2(+value.x, +value.y); }

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

template<>
inline Int2 Int2::ceil(Int2 value) { return value; }

template<>
inline Int2 Int2::floor(Int2 value) { return value; }

} // rw

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
