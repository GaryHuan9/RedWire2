#pragma once

namespace rw
{

class WireData
{
public:
	[[nodiscard]]
	bool get() const { return current_value; }

	void set() { next_value = true; }

	bool update()
	{
		bool changed = current_value != next_value;
		current_value = next_value;
		next_value = false;
		return changed;
	}

private:
	bool current_value = false;
	bool next_value = false;
};

} // rw
