#pragma once

#include "main.hpp"
#include "Utility/Types.hpp"

namespace rw
{

class Engine
{
public:
	void register_wire(Index index);

	void register_gate(Index index, Index output, const std::array<Index, 3>& inputs, bool transistor);

	void update();

private:
	std::vector<uint8_t> states, states_next;

	std::vector<Index> gates_output;
	std::vector<std::array<Index, 3>> gates_inputs;
	std::vector<bool> gates_transistor;
};

} // rw
