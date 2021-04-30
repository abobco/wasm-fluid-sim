cmake_minimum_required(VERSION 3.3)

set(PROJ_DIR  C:/Code/opengl-es)
set(BUILD_DIR build/Release)

set(XNLIB_DIR xnlib)
set(GLAD_DIR ${XNLIB_DIR}/external/glad)
set(IMGUI_DIR ${XNLIB_DIR}/external/imgui)
set(SDL_DIR ${XNLIB_DIR}/external/SDL2)
set(GLM_DIR ${XNLIB_DIR}/external/glm )

project(opengl_impl)
set( opengl_impl_src  
    ${GLAD_DIR}/src/glad.c
    ${XNLIB_DIR}/external/stb_image_impl.cpp
)
include_directories(${GLAD_DIR}/include )
add_library(opengl_impl STATIC ${opengl_impl_src} )

macro(SDL_Build_Native project_name src_file )
    project( ${project_name} ) 
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")

    set (extra_macro_args ${ARGN})

    add_executable( ${PROJECT_NAME}  
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp 
        ${IMGUI_DIR}/imgui_demo.cpp 
        ${IMGUI_DIR}/imgui_tables.cpp 
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl.h
        ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

        ${src_file} 
    )

    find_library(SDL2_LIBRARY NAME SDL2 PATHS ${SDL_DIR}/lib/x64)
    find_file(SDL2_INCLUDE_DIR NAME SDL.h PATHS ${SDL_DIR}/include HINTS SDL2 )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL_DIR}/include 
        ${GLAD_DIR}/include 
        ${IMGUI_DIR} ${IMGUI_DIR}/backends} 
    )

    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL_DIR}/lib/x64 )

    target_link_libraries( ${PROJECT_NAME} ${SDL2_LIBRARY} opengl_impl)
    target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${GLM_DIR} ${XNLIB_DIR}  ${SDL2_INCLUDE_DIR})
    target_compile_options(${PROJECT_NAME} PRIVATE /Bt)
    # target_precompile_headers(${PROJECT_NAME} PRIVATE 
    #     <iostream> <stdio.h> <fstream> <streambuf> <sstream> <stdlib.h>
    #     <ctime> <chrono> <thread> <queue> <mutex> <cmath> <unordered_map> <vector>
    #     <algorithm> <string.h> <cstring> <sys/types.h> <type_traits> <utility> <filesystem>
    #     <glm/glm.hpp> <glm/gtc/matrix_transform.hpp> <glm/gtc/type_ptr.hpp> <glm/gtx/norm.hpp> <glm/gtx/rotate_vector.hpp>
    #     <io.h> <conio.h> <errno.h>
    # )
    add_definitions(-D_USE_MATH_DEFINES)
endmacro()

SDL_Build_Native(imgui_sdl src/imgui_sdl.cpp)
SDL_Build_Native(main_sim_sdl src/main_sim_sdl.cpp)