#ifndef GPU_ABSTRACT_HDR
#define GPU_ABSTRACT_HDR

#include <array>
#include <cstdint>
#include "window.h"

class Gpu
{
	public:
        class Uniforms
        {   
            private:
            static constexpr size_t FLOAT_COUNT{1};
            static constexpr size_t INT_COUNT{1};
            static constexpr size_t UNIFORM_SIZE{4};
            std::array<float, FLOAT_COUNT> floats{};
            std::array<int32_t, INT_COUNT> ints{};

            public:
            static constexpr size_t FLOAT_SIZE{FLOAT_COUNT*UNIFORM_SIZE};
            static constexpr size_t INT_SIZE{INT_COUNT*UNIFORM_SIZE};
            static constexpr size_t SIZE{(FLOAT_COUNT+INT_COUNT)*UNIFORM_SIZE};

            const float* getFloats() {return floats.data();};
            const int* getInts() {return ints.data();};
            constexpr size_t getFloatCount() {return FLOAT_COUNT;};
            constexpr size_t getIntCount() {return INT_COUNT;};

            float &focus{floats[0]};
            int &switchView{ints[0]};
        } uniforms; 

		virtual void render() = 0;
		Gpu(Window *w, int tw, int th) : windowPtr{w}, textureWidth{tw}, textureHeight{th} {};
	protected:
		Window *windowPtr;
        int textureWidth;
        int textureHeight;
};

#endif
