#pragma once

#include "main.hpp"

#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace rw
{

template<class T>
concept Insertable = requires(std::ostream stream, T value) { stream << value; };

template<class T>
concept HasToString = requires(T value) {{ value.to_string() } -> std::same_as<std::string>; };

template<Insertable T>
std::string to_string(const T& value)
{
	std::stringstream stream;
	stream << value;
	return stream.str();
}

template<HasToString T>
std::string to_string(const T& value)
{
	return value.to_string();
}

template<class T>
std::string to_string(const T& value)
{
	return std::to_string(value);
}

template<class T>
static auto set_intersect(const std::unordered_set<T>& set0, const std::unordered_set<T>& set1)
{
	auto* search = &set0;
	auto* check = &set1;
	if (search->size() > check->size()) std::swap(search, check);

	std::unordered_set<T> result;

	for (const T& value : *search)
	{
		if (check->contains(value)) result.insert(value);
	}

	return result;
}

constexpr uint32_t make_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = std::numeric_limits<uint8_t>::max())
{
	return (static_cast<uint32_t>(red) << 24) | (static_cast<uint32_t>(green) << 16) | (static_cast<uint32_t>(blue) << 8) | alpha;
}

template<std::integral T>
constexpr T swap_endianness(T value)
{
	std::array<std::uint8_t, sizeof(T)>* pointer;
	pointer = reinterpret_cast<decltype(pointer)>(&value);
	std::reverse(pointer->begin(), pointer->end());
	return value;
}

void throw_any_gl_error();

bool imgui_begin(const char* label);

bool imgui_tooltip(const char* text);

}
