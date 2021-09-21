# Build instructions
1. mkdir build
2. cd build
3. cmake ..
4. make
5. make shader

# Usage
./lfPlayer path/to/folder float-focus-map-scale-(0.0-1.0) int-scanning-resolution 

Use mouse to move with the LF or WASD. Press Z to switch to view the focus map.

The input folder should contain image files in format x_y.png,jpg... where x is the number of row and y number of column in the LF grid.
