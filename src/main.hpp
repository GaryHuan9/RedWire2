#pragma once

#include <cmath>
#include <cassert>
#include <cstdint>

#include <queue>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

#pragma warning(disable : 4244)  // int -> float conversion

namespace rw
{

class Layer;
class TileTag;

template <class T>
class Vector2;
typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;
typedef Vector2<uint32_t> UInt2;

} // rw
