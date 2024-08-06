#pragma once

#include <cmath>
#include <cassert>
#include <cstdint>

#include <vector>
#include <string>
#include <memory>
#include <iostream>

//namespace sf
//{
//
//class Event;
//class Vertex;
//class Shader;
//class RenderWindow;
//class RenderStates;
//class RectangleShape;
//
//}

namespace rw
{

class Board;
class Layer;
class Engine;
class DataBuffer;
class VertexBuffer;
class ShaderResources;
class DrawContext;

class TileType;
class TileTag;
class TileRotation;
class Wire;
class Bridge;
class Gate;

class Application;
class Component;
class Timer;
class RenderWindow;
class Controller;
class TickControl;
class LayerView;
class Cursor;
class Debugger;

template<class T>
class Vector2;
typedef Vector2<float> Float2;
typedef Vector2<int32_t> Int2;
class Index;
class BinaryWriter;
class BinaryReader;

}
