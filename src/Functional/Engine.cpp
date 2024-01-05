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
	gates_transistor[index] = transistor ? 1 : 0;

	assert(inputs.size() == 3);
	std::copy(inputs.begin(), inputs.end(), gates_inputs[index].begin());
}

void Engine::unregister_gate(Index index)
{
	assert(index < gates_output.size());
	gates_output[index] = Index();
}

void Engine::toggle_wire_strong_powered(Index index)
{
	states[index] ^= 0b10;
}

void Engine::tick(uint32_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		for (size_t j = 0; j < states.size(); ++j)
		{
			states_next[j] = states[j] & 0b10;
		}

		for (size_t j = 0; j < gates_output.size(); ++j)
		{
			Index output = gates_output[j];
			if (not output.valid()) continue;

			bool transistor = gates_transistor[j] != 0;
			uint8_t powered = 1;

			for (Index input : gates_inputs[j])
			{
				uint8_t state = 1;
				if (input.valid()) state = states[input] != 0;

				if (transistor) powered &= state;
				else powered ^= state;
			}

			states_next[output] |= powered;
		}

		std::swap(states, states_next);
	}
}

void Engine::get_states(const void*& data, size_t& size) const
{
	data = states.data();
	size = states.size();
}

BinaryWriter& operator<<(BinaryWriter& writer, const Engine& engine)
{
	writer << engine.states;
	writer << engine.gates_output;
	writer << engine.gates_transistor;
	writer << engine.gates_inputs;
	return writer;
}

BinaryReader& operator>>(BinaryReader& reader, Engine& engine)
{
	assert(engine.states.empty());
	reader >> engine.states;
	engine.states_next.resize(engine.states.size());

	reader >> engine.gates_output;
	reader >> engine.gates_transistor;
	reader >> engine.gates_inputs;
	return reader;
}

}
