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

class Event;
class Vertex;
class RenderWindow;
class VertexBuffer;
class RenderStates;

}

namespace rw
{

class Board;
class Layer;
class DrawContext;

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
