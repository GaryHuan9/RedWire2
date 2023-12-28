#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"
#include <span>

namespace rw
{

class Engine
{
public:
	void register_wire(Index index);

	void register_gate(Index index, Index output, bool transistor, const std::span<Index>& inputs);

	void update();

	void get_states(const void*& data, size_t& size) const;

private:
	std::vector<uint8_t> states, states_next;

	std::vector<Index> gates_output;
	std::vector<bool> gates_transistor;
	std::vector<std::array<Index, 3>> gates_inputs;
};

} // rw
