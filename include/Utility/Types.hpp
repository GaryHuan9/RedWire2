#pragma once

#include "main.hpp"

namespace rw
{

template<class T>
class Vector2
{
private:
	using V = Vector2;
	using F = std::conditional_t<sizeof(T) <= sizeof(float), float, double>;

public:
	constexpr Vector2() : x{}, y{} {}

	constexpr Vector2(T x, T y) : x(x), y(y) {}

	template<class U>
	constexpr Vector2(U x, U y) : x(static_cast<T>(x)), y(static_cast<T>(y)) {}

	constexpr explicit Vector2(T value) : Vector2(value, value) {}

	template<class U>
	constexpr explicit Vector2(U value) : Vector2(value.x, value.y) {}

	constexpr T product() const { return x * y; }

	constexpr T sum() const { return x + y; }

	constexpr T dot(V value) const { return x * value.x + y * value.y; }

	constexpr T squared_magnitude() const { return dot(*this); }

	constexpr F magnitude() const { return std::sqrt(static_cast<F>(squared_magnitude())); }

	constexpr Vector2<F> normalized() const
	{
		T length = squared_magnitude();
		if (length == T()) return {};
		return Vector2<F>(*this) * (1.0f / std::sqrt(static_cast<F>(length)));
	}

	constexpr V min(V value) const { return V(std::min(x, value.x), std::min(y, value.y)); }

	constexpr V max(V value) const { return V(std::max(x, value.x), std::max(y, value.y)); }

	constexpr bool operator==(V value) const { return (x == value.x) & (y == value.y); }

	constexpr bool operator<(V value) const { return (x < value.x) & (y < value.y); }

	constexpr bool operator<=(V value) const { return (x <= value.x) & (y <= value.y); }

	constexpr bool operator!=(V value) const { return not operator==(value); }

	constexpr bool operator>(V value) const { return value.operator<(*this); }

	constexpr bool operator>=(V value) const { return value.operator<(*this); }

	constexpr V operator+() const { return V(+x, +y); }

	constexpr V operator-() const { return V(-x, -y); }

	constexpr V operator+(V value) const { return V(x + value.x, y + value.y); }

	constexpr V operator-(V value) const { return V(x - value.x, y - value.y); }

	constexpr V operator*(V value) const { return V(x * value.x, y * value.y); }

	constexpr V operator/(V value) const { return V(x / value.x, y / value.y); }

	constexpr V operator*(T value) const { return V(x * value, y * value); }

	constexpr V operator/(T value) const { return V(x / value, y / value); }

	constexpr V operator+=(V value) { return V(x += value.x, y += value.y); }

	constexpr V operator-=(V value) { return V(x -= value.x, y -= value.y); }

	constexpr V operator*=(V value) { return V(x *= value.x, y *= value.y); }

	constexpr V operator/=(V value) { return V(x /= value.x, y /= value.y); }

	constexpr V operator*=(T value) { return V(x *= value, y *= value); }

	constexpr V operator/=(T value) { return V(x /= value, y /= value); }

	static constexpr Int2 ceil(V value) { return Int2(std::ceil(value.x), std::ceil(value.y)); }

	static constexpr Int2 floor(V value) { return Int2(std::floor(value.x), std::floor(value.y)); }

	friend std::ostream& operator<<(std::ostream& stream, V value) { return stream << '(' << value.x << ", " << value.y << ')'; }

	T x;
	T y;
};

template<>
inline constexpr Int2 Int2::ceil(Int2 value) { return value; }

template<>
inline constexpr Int2 Int2::floor(Int2 value) { return value; }

class Index
{
public:
	constexpr Index() : data(static_cast<uint32_t>(-1)) {}

	constexpr explicit Index(uint32_t data) : data(data) { assert(valid()); }

	[[nodiscard]] constexpr bool valid() const { return data != Index().data; }

	[[nodiscard]]
	constexpr uint32_t value() const
	{
		assert(valid());
		return data;
	}

	constexpr operator uint32_t() const { return value(); } // NOLINT(*-explicit-constructor)

	constexpr bool operator==(Index value) const { return data == value.data; }

	constexpr bool operator!=(Index value) const { return not operator==(value); }

	friend std::ostream& operator<<(std::ostream& stream, Index value)
	{
		if (value.valid()) return stream << value.value();
		return stream << "Invalid Index";
	}

private:
	uint32_t data;
};

}

namespace std
{

template<>
struct hash<rw::Int2>
{
	size_t operator()(rw::Int2 value) const noexcept
	{
		size_t x = std::hash<int32_t>()(value.x);
		size_t y = std::hash<int32_t>()(value.y);
		return x ^ (y + 0x9E3779B9 + (x << 6) + (x >> 2));
	}
};

template<>
struct hash<rw::Index>
{
	size_t operator()(rw::Index value) const noexcept
	{
		return std::hash<uint32_t>()(value);
	}
};

}
