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

	friend std::ostream& operator<<(std::ostream& stream, V value) { return stream << '(' << value.x << ", " << value.y << ')'; }

	friend BinaryWriter& operator<<(BinaryWriter& writer, V value)
	{
		value.write(writer);
		return writer;
	}

	friend BinaryReader& operator>>(BinaryReader& reader, V& value)
	{
		value.read(reader);
		return reader;
	}

	T x{};
	T y{};

private:
	void write(BinaryWriter& writer) const;
	void read(BinaryReader& reader);
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

template<class T, class... Ts>
class TypeSet
{
public:
	template<template<class> class Wrapper>
	class Wrap;

	template<class U>
	static constexpr size_t get_index()
	{
		if constexpr (std::is_same_v<T, U>) return 0;
		else
		{
			static_assert(sizeof...(Ts) != 0, "Cannot find U.");
			return TypeSet<Ts...>::template get_index<U>() + 1;
		}
	}

	static constexpr bool are_types_unique()
	{
		constexpr bool UniqueToFront = (not std::is_same_v<T, Ts> && ...);
		if constexpr (not UniqueToFront) return false;
		else if constexpr (sizeof...(Ts) == 0) return true;
		else return TypeSet<Ts...>::are_types_unique();
	}

	static_assert(are_types_unique());
};

template<class T, class... Ts>
template<template<class> class Wrapper>
class TypeSet<T, Ts...>::Wrap
{
public:
	template<class U>
	constexpr const Wrapper<U>& get() const { return std::get<get_index<U>()>(data); }

	template<class U>
	constexpr Wrapper<U>& get() { return std::get<get_index<U>()>(data); }

	template<class Action>
	constexpr void for_each(Action action) const { for_each_index([&]<size_t I>() { action(std::get<I>(data)); }); }

	template<class Action>
	constexpr void for_each(Action action) { for_each_index([&]<size_t I>() { action(std::get<I>(data)); }); }

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Wrap& value)
	{
		value.write(writer);
		return writer;
	}

	friend BinaryReader& operator>>(BinaryReader& reader, Wrap& value)
	{
		value.read(reader);
		return reader;
	}

private:
	template<class Action, size_t I = 0>
	constexpr void for_each_index(Action action) const
	{
		action.template operator()<I>();
		if constexpr (I == sizeof...(Ts)) return;
		else for_each_index<Action, I + 1>(action);
	}

	void write(BinaryWriter& writer) const;
	void read(BinaryReader& reader);

	std::tuple<Wrapper<T>, Wrapper<Ts>...> data;
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

template<class T>
concept TrivialType = std::is_trivial_v<T>;

template<class T>
concept BinaryWritable = requires(BinaryWriter writer, T value) { writer << value; };

template<class T>
concept BinaryReadable = requires(BinaryReader reader, T value) { reader >> value; };

template<class T>
concept Serializable = BinaryWritable<T> && BinaryReadable<T>;

class BinaryWriter : NonCopyable
{
public:
	explicit BinaryWriter(std::shared_ptr<std::ostream> stream) : stream(std::move(stream)) {}

	template<TrivialType T>
	BinaryWriter& operator<<(const T& value)
	{
		write_bytes(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
		return *this;
	}

	BinaryWriter& operator<<(const std::string& value)
	{
		auto length = static_cast<uint32_t>(value.length());
		assert(length == value.length());
		*this << length;

		write_bytes(reinterpret_cast<const uint8_t*>(value.data()), length);
		return *this;
	}

	template<BinaryWritable T>
	BinaryWriter& operator<<(const std::vector<T>& vector)
	{
		auto size = static_cast<uint32_t>(vector.size());
		assert(size == vector.size());
		*this << size;

		for (const T& value : vector) *this << value;
		return *this;
	}

	template<BinaryWritable T, size_t I>
	BinaryWriter& operator<<(const std::array<T, I>& array)
	{
		for (const T& value : array) *this << value;
		return *this;
	}

private:
	void write_bytes(const uint8_t* bytes, uint32_t size)
	{
		stream->write(reinterpret_cast<const char*>(bytes), size);
		if (stream->bad()) throw std::out_of_range("Insertion error.");
	}

	std::shared_ptr<std::ostream> stream;
};

class BinaryReader : NonCopyable
{
public:
	explicit BinaryReader(std::shared_ptr<std::istream> stream) : stream(std::move(stream)) {}

	template<TrivialType T>
	BinaryReader& operator>>(T& value)
	{
		read_bytes(reinterpret_cast<uint8_t*>(&value), sizeof(T));
		return *this;
	}

	BinaryReader& operator>>(std::string& value)
	{
		assert(value.empty());

		uint32_t length;
		*this >> length;

		value.resize(length);
		read_bytes(reinterpret_cast<uint8_t*>(value.data()), length);
		return *this;
	}

	template<BinaryReadable T>
	BinaryReader& operator>>(std::vector<T>& vector)
	{
		assert(vector.empty());

		uint32_t size;
		*this >> size;

		vector.resize(size);
		for (T& value : vector) *this >> value;
		return *this;
	}

	template<BinaryReadable T, size_t I>
	BinaryReader& operator>>(std::array<T, I>& array)
	{
		for (T& value : array) *this >> value;
		return *this;
	}

private:
	void read_bytes(uint8_t* bytes, uint32_t size)
	{
		stream->read(reinterpret_cast<char*>(bytes), size);
		if (stream->eof()) throw std::out_of_range("Stream exhausted.");
	}

	std::shared_ptr<std::istream> stream;
};

template<class T>
void Vector2<T>::write(BinaryWriter& writer) const { writer << x << y; }

template<class T>
void Vector2<T>::read(BinaryReader& reader) { reader >> x >> y; }

inline BinaryWriter& operator<<(BinaryWriter& writer, Index value)
{
	++value.data;
	writer << value.data;
	return writer;
}

inline BinaryReader& operator>>(BinaryReader& reader, Index& value)
{
	reader >> value.data;
	--value.data;
	return reader;
}

template<class T, class... Ts>
template<template<class> class Wrapper>
void TypeSet<T, Ts...>::Wrap<Wrapper>::write(BinaryWriter& writer) const
{
	auto write = [&writer]<class U>(const U& value) { writer << value; };
	for_each(write);
}

template<class T, class... Ts>
template<template<class> class Wrapper>
void TypeSet<T, Ts...>::Wrap<Wrapper>::read(BinaryReader& reader)
{
	auto read = [&reader]<class U>(U& value) { reader >> value; };
	for_each(read);
}

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
