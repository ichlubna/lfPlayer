# Build instructions
1. mkdir build
2. cd build
3. cmake ..
4. make
5. make shader

# Light Field Player
Vulkan-based GPU light field viewer with interpolation between the input frames.

## Usage
_lfPlayer -i /MyAmazingMachine/thatScene.lf -s 1.0 -q 256_

-i input path - can be file encoded by lfEncoder or folder with images in the same format as in lfEncoder  
-s focus map scale - how big will the focus map be, in range <0;1>, higher = nicer but slower  
-q number of scanning iterations - how densely will the image offsets be scanned when generating focus map, higher = nicer but slower  

Use mouse to move with the light field or keyboard keys _WASD_. Press _Z_ to switch to view the focus map.

If the encoded file is used as the input, GPU video decoders are utilized to decode the specified format in the file. The used GPU has to support HW-accelerated decoding of the given format.

# Light Field Encoder
Encodes light field image grid into a video stream. The encoded files can be directly used in the Light Field Player as the optimal way to stream light field data in real-time.

## Usage
_lfEncoder -i /MyAmazingMachine/thoseImages -q 1.0 -f H265 -o ./coolFile.lf_  

-i input path - the input folder should contain image files in format x\_y.png,jpg... where x is the number of row and y number of column in the LF grid  
-q encoding quality - normalized quality parameter for the given format in range <0;1>, higher = nicer but larger  
-f video format - format used to encode the images: H265 or AV1  
-o output file

## Encoded .lf file specification
The file is in binary format with _.lf_ extension and consists of:  
HEADER **|** OFFSETS **|** PACKETS

HEADER consists of 32-bit unsigned int values:  
IMAGE WIDTH [PX] **|** IMAGE HEIGHT [PX] **|** GRID COLUMNS [IMAGES] **|** GRID ROWS [IMAGES] **|** KEY FRAME POSITION X [IMAGES] **|** KEY FRAME POSITION Y [IMAGES] **|** VIDEO FORMAT (0 = H265, 1 = AV1)

OFFSETS is an array of 32-bit unsigned int values indicating the beginning of the given packet in the PACKETS section. Example: offsets[0] is the byte address of the first packet data in the file.

PACKETS is a byte array containing the video packets data. The packets are stored in column-major order so the packets from the grid are stored as:  
0\_0 **|** 1\_0 **|** 2\_0 **|** ... **|** 0\_1 **|** 0\_2 **|** ...

# Related publications
CHLUBNA Tomáš, MILET Tomáš and ZEMČÍK Pavel. Real-time per-pixel focusing method for light field rendering. Computational Visual Media, vol. 2021, no. 7, pp. 319-333. ISSN 2096-0662.

# Used libraries
- [STB Image](https://github.com/nothings/stb) 
- [FFmpeg](https://ffmpeg.org) 
- [GLM](https://github.com/g-truc/glm) 
- [Vulkan HPP](https://github.com/KhronosGroup/Vulkan-Hpp) 
- [GLSLC](https://github.com/google/shaderc/tree/main/glslc) 
- [GLFW](https://www.glfw.org) 
