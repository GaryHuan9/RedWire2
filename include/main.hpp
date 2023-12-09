#pragma once

#include <cmath>
#include <cassert>
#include <cstdint>

#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace sf
{

class RenderWindow;
class Vertex;

}

namespace rw
{

class Chunk;
class Layer;
class TileTag;
class WireData;

template<class T>
class Vector2;
typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;

} // rw
