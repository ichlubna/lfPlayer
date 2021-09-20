#include "inputs.h"
#include<iostream>
void Inputs::setMousePosition(double x, double y)
{
    mouseX = x; 
    mouseY = y; 
    mouseChanged = true;
}

bool Inputs::mouseMoved()
{
    bool move = mouseChanged;
    mouseChanged = false;
    return move;
}

Inputs::mousePosition Inputs::getMousePosition() const
{
    return {mouseX, mouseY};
}

double Inputs::getMouseScroll() const
{
    return mouseScroll;
}

void Inputs::setMouseScroll(double s)
{
    mouseScroll = s;
}

void Inputs::press(Key key)
{
    keys |= key;
}

void Inputs::release(Key key)
{
    keys &= ~key;
}

