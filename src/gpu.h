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
            static constexpr size_t LF_ATTRIBS{16};
            static constexpr size_t STANDALONE_COUNT{2};
            static constexpr size_t UNIFORM_COUNT{LF_ATTRIBS*3+STANDALONE_COUNT};
            using Data = std::array<int32_t, UNIFORM_COUNT>; 
           
            size_t index{0}; 
            float* mapFloat(){return reinterpret_cast<float*>(&data[index++]);}
            int* mapInt(){return &data[index++];}
            Data data{};

            public:
            static constexpr size_t SIZE{UNIFORM_COUNT*sizeof(int32_t)};
            const Data* getData() {return &data;};
            
            float *focus;
            int *switchView;
            std::array<float*, LF_ATTRIBS> lfAttribs;
            
            Uniforms()
            {
                for(auto &attrib : lfAttribs) 
                    attrib = mapFloat();
                focus = mapFloat();
                switchView = mapInt();
            };

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
            glm::vec2 offset;
            bool changed{true};
        };

        virtual void loadFrameTextures(Resources::ImageGrid &images) = 0;
        void updateFrameIndices(std::vector<LfCurrentFrame> &frames);
		virtual void render() = 0;
		Gpu(Window *w, LfInfo lf) : windowPtr{w}, lfInfo{lf}{};
	protected:
		Window *windowPtr;
        LfInfo lfInfo;
        std::vector<LfCurrentFrame> currentFrames{4};
};

#endif
