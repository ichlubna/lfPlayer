#include "bindingsHeader.glsl"
#include "generalHeader.glsl"

layout(constant_id = 1) const int LOCAL_SIZE_X = 16;
layout(constant_id = 2) const int LOCAL_SIZE_Y = 16;
layout(local_size_x_id = 1, local_size_y_id = 2) in;

ivec2 outCoord = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
vec2 mapUV =  clamp(vec2(outCoord+0.5f)/vec2(MAP_WIDTH,MAP_HEIGHT), 0.0, 1.0);

mat4 sampleImage(vec2 coord, int textID)
{
    vec4 c = texture(sampler2D(textures[textID+LF_FRAME_TEXTURE_OFFSET], textSampler), coord);
    mat4 block;
    for(int i=0; i<4; i++)
        block[i] = texture(sampler2D(textures[textID+2], textSampler), coord+(vec2(1.0f,1.01f)-2*vec2(i/2, i%2))*LF_HALF_PX_SIZE*3);
    block[0][3] = c.r;
    block[1][3] = c.g;
    block[2][3] = c.b;
    return block;
}
