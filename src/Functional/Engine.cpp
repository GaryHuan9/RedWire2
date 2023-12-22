#include "Functional/Engine.hpp"
#include "Functional/Board.hpp"
#include "Functional/Tiles.hpp"

namespace rw
{

void Engine::register_wire(Index index)
{
	if (index == states.size())
	{
		states.resize(index + 1);
		states_next.resize(index + 1);
	}

	assert(index < states.size());
	states[index] = 0;
}

void Engine::register_gate(Index index, Index output, const std::array<Index, 3>& inputs, bool transistor)
{
	if (index == gates_output.size())
	{
		gates_output.resize(index + 1);
		gates_inputs.resize(index + 1);
		gates_transistor.resize(index + 1);
	}

	assert(index < states.size());

	gates_output[index] = output;
	gates_inputs[index] = inputs;
	gates_transistor[index] = transistor;
}

void Engine::update()
{
	for (size_t i = 0; i < gates_output.size(); ++i)
	{
		Index output = gates_output[i];
		if (output == Index()) continue;

		bool transistor = gates_transistor[i];
		uint8_t result = 1;

		for (Index input : gates_inputs[i])
		{
			uint8_t value = input == Index() ? 1 : states[input];

			if (transistor) result &= value;
			else result ^= value;
		}

		states_next[i] = result;
	}

	std::swap(states, states_next);
}

}
