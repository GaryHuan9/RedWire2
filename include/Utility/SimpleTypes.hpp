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

	constexpr V operator+=(V value) { return V(x += value.x, y += value.y); }

	constexpr V operator-=(V value) { return V(x -= value.x, y -= value.y); }

	constexpr V operator*=(V value) { return V(x *= value.x, y *= value.y); }

	constexpr V operator/=(V value) { return V(x /= value.x, y /= value.y); }

	constexpr V operator*=(T value) { return V(x *= value, y *= value); }

	constexpr V operator/=(T value) { return V(x /= value, y /= value); }

	static constexpr Int2 ceil(V value) { return Int2(std::ceil(value.x), std::ceil(value.y)); }

	static constexpr Int2 floor(V value) { return Int2(std::floor(value.x), std::floor(value.y)); }

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

	T x;
	T y;

private:
	void write(BinaryWriter& writer) const;
	void read(BinaryReader& reader);
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

private:
	std::tuple<Wrapper<T>, Wrapper<Ts>...> data;
};

template<class T>
concept TrivialType = std::is_trivial_v<T>;

template<class T>
concept BinaryWritable = requires(BinaryWriter writer, T value) { writer << value; };

template<class T>
concept BinaryReadable = requires(BinaryReader reader, T value) { reader >> value; };

template<class T>
concept Serializable = BinaryWritable<T> && BinaryReadable<T>;

class BinaryWriter
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

class BinaryReader
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

inline BinaryWriter& operator<<(BinaryWriter& writer, Index value) { return writer << (value.data + 1); }

inline BinaryReader& operator>>(BinaryReader& reader, Index& value)
{
	reader >> value.data;
	--value.data;
	return reader;
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
