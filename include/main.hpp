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
class Shader;
class RenderWindow;
class RenderStates;

}

namespace rw
{

class Board;
class Layer;
class Engine;
class DrawContext;
class DataBuffer;
class VertexBuffer;

class TileType;
class TileTag;
class TileRotation;
class Wire;
class Bridge;
class Gate;

class Application;
class Component;
class Timer;
class LayerView;
class Controller;
class Debugger;

template<class T>
class Vector2;
typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;

}
