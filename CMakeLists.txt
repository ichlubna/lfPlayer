cmake_minimum_required(VERSION 3.20)
project(lfPlayer LANGUAGES CXX)
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 0)

set(GLSLC_BINARY "glslc" CACHE PATH "Path to glslc binary to compile shaders by \"make shader\"")
set(ASTYLE_BINARY "astyle" CACHE PATH "Path to astyle binary to format the code by \"make format\"")
set(ASTYLE_OPTIONS -A1 -C -n -k3 -p -U -xg CACHE STRING "Astyle options used by \"make format\"")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

set(SOURCES 
src/lfPlayerApp.cpp
src/gpu.cpp
src/gpuVulkan.cpp
src/windowGlfw.cpp
src/simulation.cpp
src/camera.cpp
src/inputs.cpp
src/resources.cpp
src/analyzer.cpp
src/muxing.cpp
src/libs/loadingBar/loadingbar.hpp
src/libs/arguments/arguments.hpp
)

set(SOURCES_ENCODER
src/lfEncoderApp.cpp
src/encoder.cpp
src/muxing.cpp
src/libs/loadingBar/loadingbar.hpp
src/analyzer.cpp
src/libs/arguments/arguments.hpp
)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(FFMPEG REQUIRED)
add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE ${FFMPEG_LIBRARIES})
target_include_directories(ffmpeg INTERFACE $<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS})

set(ENCODER lfEncoder)
add_executable(${ENCODER} ${SOURCES_ENCODER})

add_executable(${PROJECT_NAME} ${SOURCES})
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/libs)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/libs)

set(SHADER_SRC_PATH src/shader)
set(SHADER_COMPILATION_PATH precompiled)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/precompiled)

set(SHADER_LIST
fragment.frag
vertex.vert
computeFocusMap.comp
computeLightfield.comp
computeRange.comp
)

add_custom_target(shader WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
foreach(SHADER_NAME ${SHADER_LIST})
    cmake_path(GET SHADER_NAME STEM STEM_NAME)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_SOURCE_DIR}/${SHADER_COMPILATION_PATH}/${STEM_NAME}.spv)
    add_custom_command(TARGET shader COMMAND ${GLSLC_BINARY} ${CMAKE_SOURCE_DIR}/${SHADER_SRC_PATH}/${SHADER_NAME} -o ${CMAKE_SOURCE_DIR}/${SHADER_COMPILATION_PATH}/${STEM_NAME}.spv -O --target-env=vulkan1.2)
endforeach()

add_custom_target(format WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
add_custom_command(TARGET format COMMAND ${ASTYLE_BINARY} ${CMAKE_SOURCE_DIR}/src/*.cpp,*.h \${ASTYLE_OPTIONS})
add_custom_command(TARGET format COMMAND ${ASTYLE_BINARY} ${CMAKE_SOURCE_DIR}/${SHADER_SRC_PATH}/*.glsl,*.frag,*.comp,*.vert \${ASTYLE_OPTIONS})

target_link_libraries(${PROJECT_NAME} vulkan glfw glm)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(${ENCODER} ffmpeg swscale)
target_compile_features(${ENCODER} PRIVATE cxx_std_20)
target_compile_options(${ENCODER} PRIVATE -Wall -Wextra -pedantic)

