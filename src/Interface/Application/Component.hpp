#pragma once

namespace rw {

class Application;

class Component
{
public:
    explicit Component(Application& application);

    virtual ~Component() = default;

    virtual void initialize() = 0;

    virtual void update() = 0;
    virtual void render() = 0;

protected:
    Application& application;
};


} // rw
