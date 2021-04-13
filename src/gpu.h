#ifndef GPU_ABSTRACT_HDR
#define GPU_ABSTRACT_HDR

#include "window.h"

class Gpu
{
	public:
		virtual void render() = 0;
		Gpu(Window *w, int, int) : windowPtr{w} {};
	protected:
		Window *windowPtr;
};

#endif
