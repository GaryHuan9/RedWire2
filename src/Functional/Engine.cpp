#include "Functional/Engine.hpp"

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

void Engine::register_gate(Index index, Index output, bool transistor, const std::span<Index>& inputs)
{
	if (index == gates_output.size())
	{
		gates_output.resize(index + 1);
		gates_inputs.resize(index + 1);
		gates_transistor.resize(index + 1);
	}

	assert(index < gates_output.size());
	gates_output[index] = output;
	gates_transistor[index] = transistor;

	assert(inputs.size() == 3);
	std::copy(inputs.begin(), inputs.end(), gates_inputs[index].begin());
}

void Engine::unregister_gate(Index index)
{
	assert(index < gates_output.size());
	gates_output[index] = Index();
}

void Engine::tick(uint32_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		for (size_t j = 0; j < gates_output.size(); ++j)
		{
			Index output = gates_output[j];
			if (output == Index()) continue;

			bool transistor = gates_transistor[j];
			uint8_t result = 1;

			for (Index input : gates_inputs[j])
			{
				uint8_t value = input == Index() ? 1 : states[input];

				if (transistor) result &= value;
				else result ^= value;
			}

			states_next[output] |= result;
		}

		std::swap(states, states_next);
		std::fill(states_next.begin(), states_next.end(), 0);
	}
}

void Engine::get_states(const void*& data, size_t& size) const
{
	data = states.data();
	size = states.size();
}

}
