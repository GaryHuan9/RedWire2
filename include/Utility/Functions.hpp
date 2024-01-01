#pragma once

#include "main.hpp"

#include <sstream>
#include <unordered_set>

namespace rw
{

template<class T>
concept Insertable = requires(std::ostream stream, T value) { stream << value; };

template<class T>
concept HasToString = requires(T value) {{ value.to_string() } -> std::convertible_to<std::string>; };

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
