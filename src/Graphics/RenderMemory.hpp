#pragma once
#include <type_traits>
#include <bgfx/bgfx.h>

namespace rw
{
template<class T> requires std::is_trivial_v<T>
class RenderMemory
{
public:
    void push_back(const T& value)
    {
        if (current_size == current_capacity) grow(current_size + 1);

        new (current_data + current_size) T(value);
        ++current_size;
    }

    const bgfx::Memory* refer() const
    {
        return bgfx::makeRef(current_data, current_size * sizeof(T), ???, ???  );
    }

private:
    void grow();

    T* current_data{};
    uint32_t current_size{};
    uint32_t current_capacity{};
};
}
