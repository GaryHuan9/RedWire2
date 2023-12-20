#pragma once

#include "main.hpp"

#include <sstream>

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

}
