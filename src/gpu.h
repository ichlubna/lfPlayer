#ifndef GPU_ABSTRACT_HDR
#define GPU_ABSTRACT_HDR

#include "window.h"

class Gpu
{
	public:
		virtual void render() = 0;
		Gpu(Window *w, int tw, int th) : windowPtr{w}, textureWidth{tw}, textureHeight{th} {};
	protected:
		Window *windowPtr;
        int textureWidth;
        int textureHeight;
};

#endif
