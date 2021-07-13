layout(constant_id = 0) const int MAX_TEXTURES = 64;
layout(constant_id = 3) const int ROWS = 8;
layout(constant_id = 4) const int COLS = 8;
layout(constant_id = 5) const int WIDTH = 1920;
layout(constant_id = 6) const int HEIGHT = 1080;
layout(constant_id = 7) const float ASPECT_RATIO = 1.78;
layout(constant_id = 8) const float PX_SIZE_X = 0.0005;
layout(constant_id = 9) const float PX_SIZE_Y = 0.0009;
layout(constant_id = 10) const float HALF_PX_SIZE_X = 0.00025;
layout(constant_id = 11) const float HALF_PX_SIZE_Y = 0.00045;
vec2 PX_SIZE = vec2(PX_SIZE_X, PX_SIZE_Y);
vec2 HALF_PX_SIZE = vec2(HALF_PX_SIZE_X, HALF_PX_SIZE_Y);

layout(set = 0, binding = 0) uniform uniformBuffer
{
    mat4 lfFrameAttribs;
    float focus;
    int switchView;
} uniforms;

layout(set = 0, binding = 1) uniform sampler textSampler;
layout(set = 0, binding = 2, rgba8) uniform image2D images[MAX_TEXTURES];
layout(set = 0, binding = 3) uniform texture2D textures[MAX_TEXTURES];
const int OUTPUT_TEXTURE_ID = 0;
const int FOCUSMAP_TEXTURE_ID = 1;

float lfFrameWeight(int i)
{
    return uniforms.lfFrameAttribs[i][0];
}

vec2 lfFrameOffset(int i)
{
    return uniforms.lfFrameAttribs[i].yz;
}


