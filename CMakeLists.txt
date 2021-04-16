cmake_minimum_required(VERSION 3.20)
project(lfPlayer LANGUAGES CXX)
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 0)

set(GLSLC_BINARY "glslc" CACHE PATH "Path to glslc binary to compile shaders by \"make shader\"")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

set(SOURCES 
src/main.cpp
src/gpuVulkan.cpp
src/windowGlfw.cpp
src/simulation.cpp
src/camera.cpp
src/inputs.cpp
)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_target(run 	COMMAND ${PROJECT_NAME}
						DEPENDS ${PROJECT_NAME}
						WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})


set(SHADER_SRC_PATH src/shader)
set(SHADER_COMPILATION_PATH precompiled)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/precompiled)

set(SHADER_LIST
fragment.frag
vertex.vert
computeFocusMap.comp
)

add_custom_target(shader WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
foreach(SHADER_NAME ${SHADER_LIST})
    cmake_path(GET SHADER_NAME STEM STEM_NAME)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_SOURCE_DIR}/${SHADER_COMPILATION_PATH}/${STEM_NAME}.spv)
    add_custom_command(TARGET shader COMMAND ${GLSLC_BINARY} ${CMAKE_SOURCE_DIR}/${SHADER_SRC_PATH}/${SHADER_NAME} -o ${CMAKE_SOURCE_DIR}/${SHADER_COMPILATION_PATH}/${STEM_NAME}.spv -O)
endforeach()



#add_custom_target(shader    COMMAND ${GLSLC_BINARY} ${CMAKE_SOURCE_DIR}/src/shader/vertex.vert -o ${CMAKE_SOURCE_DIR}/precompiled/vertex.spv -O
#                            COMMAND ${GLSLC_BINARY} ${CMAKE_SOURCE_DIR}/src/shader/fragment.frag -o ${CMAKE_SOURCE_DIR}/precompiled/fragment.spv -O
#    						WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
target_link_libraries(${PROJECT_NAME} vulkan glfw glm)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
