#pragma once

#include "main.hpp"
#include "Utility/SimpleTypes.hpp"

#include <span>

namespace rw
{

class Engine
{
public:
	void register_wire(Index index);

	void register_gate(Index index, Index output, bool transistor, const std::span<Index>& inputs);

	void unregister_gate(Index index);

	void tick(uint32_t count = 1);

	void get_states(const void*& data, size_t& size) const;

	friend BinaryWriter& operator<<(BinaryWriter& writer, const Engine& engine);
	friend BinaryReader& operator>>(BinaryReader& reader, Engine& engine);

private:
	std::vector<uint8_t> states, states_next;

	std::vector<Index> gates_output;
	std::vector<uint8_t> gates_transistor;
	std::vector<std::array<Index, 3>> gates_inputs;
};

} // rw
