#ifndef GPU_ABSTRACT_HDR
#define GPU_ABSTRACT_HDR

#include <array>
#include <cstdint>
#include <memory>
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

        class LfCurrentFrame
        {
            public:
            glm::ivec2 coords;
            float weight{0};
            glm::vec2 offset;
        };

        class FocusMapSettings
        {
            public:
            float scale{1};
            size_t iterations{256};
            size_t width{1920}, height{1080};
        };

        void updateFrameIndices(std::vector<LfCurrentFrame> &frames);
		virtual void render() = 0;
		Gpu(Window *w, std::shared_ptr<Resources::FrameGrid> lf, FocusMapSettings fs) : windowPtr{w}, lightfield{lf}, focusMapSettings{fs}
        {
            focusMapSettings.width = lightfield->width*focusMapSettings.scale;
            focusMapSettings.height = lightfield->height*focusMapSettings.scale;
        };
	protected:
		Window *windowPtr;
        std::shared_ptr<Resources::FrameGrid> lightfield;
        FocusMapSettings focusMapSettings;
        std::vector<LfCurrentFrame> currentFrames{4};
};

#endif
