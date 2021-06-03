#ifndef GPU_ABSTRACT_HDR
#define GPU_ABSTRACT_HDR

#include <array>
#include <cstdint>
#include "window.h"
#include "resources.h"

class Gpu
{
	public:
        class Uniforms
        {   
            private:
            static constexpr size_t UNIFORM_COUNT{6};
            using Data = std::array<int32_t, UNIFORM_COUNT>; 
            Data data{};

            public:
            static constexpr size_t SIZE{UNIFORM_COUNT*sizeof(int32_t)};

            const Data* getData() {return &data;};

            float &focus{reinterpret_cast<float&>(data[0])};
            int &switchView{data[1]};
            std::array<float*, 4> lfWeights{    reinterpret_cast<float*>(&data[2]),
                                                reinterpret_cast<float*>(&data[3]),
                                                reinterpret_cast<float*>(&data[4]),
                                                reinterpret_cast<float*>(&data[5])};

        } uniforms; 

        class LfInfo
        {
            public:
            unsigned int width{1920};
            unsigned int height{1080};
            unsigned int rows{8};
            unsigned int cols{8};
        };

        class LfCurrentFrame
        {
            public:
            unsigned int index{0};
            float weight{0};
            bool changed{true};
        };

        virtual void loadFrameTextures(Resources::ImageGrid images) = 0;
        void updateFrameIndices(std::vector<LfCurrentFrame> &frames);
		virtual void render() = 0;
		Gpu(Window *w, LfInfo lf) : windowPtr{w}, lfInfo{lf}{};
	protected:
		Window *windowPtr;
        LfInfo lfInfo;
        std::vector<LfCurrentFrame> currentFrames{4};
};

#endif
