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
class Event;

}

namespace rw
{

class Board;
class Layer;

class Chunk;
class TileTag;
enum class TileType : uint8_t;

class Wire;
class Bridge;

class Application;
class Component;
class Timer;
class LayerView;
class Controller;

template<class T>
class Vector2;
typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;

}
