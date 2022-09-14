layout(constant_id = 0) const int MAX_TEXTURES = 10;
layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(constant_id = 3) const int LF_WIDTH = 1920;
layout(constant_id = 4) const int LF_HEIGHT = 1080;
layout(constant_id = 5) const float ASPECT_RATIO = 1920 / 1080.0f;
layout(constant_id = 6) const float LF_HALF_PX_SIZE_X = 1 / 1920.0f;
layout(constant_id = 7) const float LF_HALF_PX_SIZE_Y = 1 / 1080.0f;
layout(constant_id = 8) const float MAP_HALF_PX_SIZE_X = 1 / 1920.0f;
layout(constant_id = 9) const float MAP_HALF_PX_SIZE_Y = 1 / 1080.0f;
layout(constant_id = 10) const int SAMPLE_COUNT = 255;
layout(constant_id = 11) const int MAP_WIDTH = 1920;
layout(constant_id = 12) const int MAP_HEIGHT = 1080;

//must be same as in gpuVulkan for screenshots
const int SHADER_STORAGE_RANGE_SIZE = 1000;

vec2 LF_HALF_PX_SIZE = vec2(LF_HALF_PX_SIZE_X, LF_HALF_PX_SIZE_Y);
vec2 MAP_HALF_PX_SIZE = vec2(MAP_HALF_PX_SIZE_X, MAP_HALF_PX_SIZE_Y);

layout(set = 0, binding = 0) uniform uniformBuffer
{
    mat4 lfFrameAttribs;
    float focus;
    int switchView;
    int screenshot;
    int windowWidth;
    int windowHeight;
} uniforms;

layout(std430, set = 0, binding = 4) buffer volatile shaderStorageBuffer
{
    int rangeData[SHADER_STORAGE_RANGE_SIZE];
    int data[];
} shaderStorage;

layout(set = 0, binding = 1) uniform sampler textSampler;
layout(set = 0, binding = 2, rgba8) uniform image2D images[MAX_TEXTURES];
layout(set = 0, binding = 3) uniform texture2D textures[MAX_TEXTURES];
const int OUTPUT_TEXTURE_ID = 0;
const int FOCUSMAP_TEXTURE_ID = 1;
const int LF_FRAME_TEXTURE_OFFSET = 2;

float lfFrameWeight(int i)
{
    return uniforms.lfFrameAttribs[i][0];
}

vec2 lfFrameOffset(int i)
{
    return uniforms.lfFrameAttribs[i].yz;
}


