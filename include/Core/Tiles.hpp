#pragma once

namespace rw
{

class Wire
{
	//public:
	//	[[nodiscard]]
	//	bool get() const { return value != 0; }
	//
	//	void set() { value ^= 1; }
	//
	//	void
	//
	//private:
	//	uint8_t value = false;
public:
	explicit Wire(uint32_t color) : color(color) {}

	uint32_t color;
};

class Bridge
{
public:
private:
};

}
