#pragma once

#include "main.hpp"

#include <cmath>
#include <iostream>
#include <vector>

namespace rw
{

template<class T>
class Vector2
{
	using V = Vector2;
	using F = std::conditional_t<sizeof(T) <= sizeof(float), float, double>;

public:
	constexpr Vector2() = default;

	constexpr Vector2(T x, T y) : x(x), y(y) {}

	template<class U>
	constexpr Vector2(U x, U y) : Vector2(static_cast<T>(x), static_cast<T>(y)) {}

	constexpr explicit Vector2(T value) : Vector2(value, value) {}

	template<class U>
	constexpr explicit Vector2(U value) : Vector2(value.x, value.y) {}

	constexpr T product() const { return x * y; }

	constexpr T sum() const { return x + y; }

	constexpr T dot(V value) const { return x * value.x + y * value.y; }

	constexpr T magnitude2() const { return dot(*this); }

	constexpr F magnitude() const { return std::sqrt(static_cast<F>(magnitude2())); }

	constexpr Vector2<F> normalized() const
	{
		T length = magnitude2();
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

	constexpr V& operator+=(V value) { return *this = (*this + value); }

	constexpr V& operator-=(V value) { return *this = (*this - value); }

	constexpr V& operator*=(V value) { return *this = (*this * value); }

	constexpr V& operator/=(V value) { return *this = (*this / value); }

	constexpr V& operator*=(T value) { return *this = (*this * value); }

	constexpr V& operator/=(T value) { return *this = (*this / value); }

	static constexpr Int2 ceil(V value) { return Int2(std::ceil(value.x), std::ceil(value.y)); }

	static constexpr Int2 floor(V value) { return Int2(std::floor(value.x), std::floor(value.y)); }

	static constexpr Int2 round(V value) { return Int2(std::lround(value.x), std::lround(value.y)); }

	friend std::ostream& operator<<(std::ostream& stream, V value) {
          return stream << '(' << value.x << ", " << value.y << ')'; }

	T x{};
	T y{};
};

template<>
inline constexpr Int2 Int2::ceil(Int2 value) { return value; }

template<>
inline constexpr Int2 Int2::floor(Int2 value) { return value; }

template<>
inline constexpr Int2 Int2::round(Int2 value) { return value; }

class Bounds
{
public:
	class Iterator;

	Bounds() = default;

	Bounds(Int2 min, Int2 max) : min(min), max(max) { assert(max >= min); }

	explicit Bounds(Int2 min) : Bounds(min, min + Int2(1)) {}

	Bounds(Float2 min, Float2 max) : Bounds(Float2::floor(min), Float2::ceil(max)) {}

	[[nodiscard]] Int2 get_min() const { return min; }

	[[nodiscard]] Int2 get_max() const { return max; }

	[[nodiscard]] Int2 size() const { return max - min; }

	[[nodiscard]] Float2 extend() const { return Float2(size()) / 2.0f; }

	[[nodiscard]] Float2 center() const { return Float2(min + max) / 2.0f; }

	[[nodiscard]] bool contains(Int2 position) const { return min <= position && position < max; }

	[[nodiscard]] Iterator begin() const;
	[[nodiscard]] Iterator end() const;

	static Bounds encapsulate(Int2 point0, Int2 point1)
	{
		Int2 min = point0.min(point1);
		Int2 max = point0.max(point1);
		return { min, max + Int2(1) };
	}

	static Bounds encapsulate(Bounds bounds0, Bounds bounds1)
	{
		Int2 min = bounds0.min.min(bounds1.min);
		Int2 max = bounds0.max.max(bounds1.max);
		return { min, max };
	}

	Bounds operator+(Int2 value) const { return { min + value, max + value }; }

	Bounds operator-(Int2 value) const { return { min - value, max - value }; }

	Bounds& operator+=(Int2 value) { return *this = *this + value; }

	Bounds& operator-=(Int2 value) { return *this = *this - value; }

private:
	Int2 min;
	Int2 max;
};

class Bounds::Iterator
{
public:
	Iterator(int32_t min_x, int32_t max_x, Int2 current) : min_x(min_x), max_x(max_x), current(current) {}

	Int2 operator*() const { return current; }

	Iterator& operator++()
	{
		if (++current.x == max_x)
		{
			current.x = min_x;
			++current.y;
		}

		return *this;
	}

	bool operator==(Iterator other) const { return current == other.current; }

private:
	int32_t min_x;
	int32_t max_x;
	Int2 current;
};

inline Bounds::Iterator Bounds::begin() const { return { min.x, max.x, min }; }

inline Bounds::Iterator Bounds::end() const { return { min.x, max.x, Int2(min.x, max.y) }; }

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

	friend BinaryWriter& operator<<(BinaryWriter& writer, Index value);
	friend BinaryReader& operator>>(BinaryReader& reader, Index& value);

private:
	uint32_t data;
};

class NonCopyable
{
public:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;

protected:
	NonCopyable() = default;
	~NonCopyable() = default;
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
