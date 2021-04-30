cmake_minimum_required(VERSION 3.3)

set(PROJ_DIR  ..)
set(SRC_DIR ${PROJ_DIR}/src)
set(BUILD_DIR html)
set(XNLIB_DIR ${PROJ_DIR}/xnlib)

set(GLAD_DIR ${XNLIB_DIR}/external/glad)
set(SDL_DIR ${XNLIB_DIR}/external/SDL2)
set(GLM_DIR ${XNLIB_DIR}/external/glm )

set(EM_SHELL_FILE ${BUILD_DIR}/shell_minimal.html)
set(FILE_ASSETS 
    assets/shaders/gles100/basic.vs 
    assets/shaders/gles100/basic.fs

    assets/shaders/gles100/screen.vs 
    assets/shaders/gles100/screen.fs
    assets/shaders/gles100/particle_post.fs

    assets/shaders/gles300/basic.vs 
    assets/shaders/gles300/basic.fs
    
    assets/shaders/gles300/instanced.vs 
    assets/shaders/gles300/instanced.fs
    
    assets/shaders/gles300/screen.vs 
    assets/shaders/gles300/screen.fs
    assets/shaders/gles300/particle_post.fs
    
    assets/xn_particle_config.json
)

project(opengl_impl)
set( opengl_impl_src  
    ${GLAD_DIR}/src/glad.c
    ${XNLIB_DIR}/external/stb_image_impl.cpp
)
include_directories(${GLAD_DIR}/include )
add_library(opengl_impl STATIC ${opengl_impl_src} )

macro(SDL_Build_EMCC project_name src_file ) 
    project( ${project_name} ) 
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set (extra_macro_args ${ARGN})

    add_executable(${PROJECT_NAME}  
        ${XNLIB_DIR}/external/imgui.h
        ${XNLIB_DIR}/external/imgui.cpp
        ${XNLIB_DIR}/external/imgui_demo.cpp
        ${XNLIB_DIR}/external/imgui_draw.cpp
        ${XNLIB_DIR}/external/imgui_impl_sdl.h
        ${XNLIB_DIR}/external/imgui_impl_sdl.cpp

        ${src_file} 
    )

    # target_precompile_headers(${PROJECT_NAME} PRIVATE 
    #     <iostream> <stdio.h> <fstream> <streambuf> <sstream> <stdlib.h>
    #     <ctime> <chrono> <thread> <queue> <mutex> <cmath> <unordered_map> <vector>
    #     <algorithm> <string.h> <cstring> <sys/types.h> <type_traits> <utility> <filesystem>
    #     <glm/glm.hpp> <glm/gtc/matrix_transform.hpp> <glm/gtc/type_ptr.hpp> <glm/gtx/norm.hpp> <glm/gtx/rotate_vector.hpp>
    # )

    set_target_properties( ${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
    )
    set(emcc_compiler_flags "")
    set(emcc_linker_flags "-s USE_SDL=2")
    # set(emcc_compiler_flags "${emcc_compiler_flags} -s -O3 -std=c++17 --shell-file ${CMAKE_BINARY_DIR}/emscripten/shell_minimal.html")
    set(emcc_compiler_flags "${emcc_compiler_flags} -s -O3 -std=c++17 --shell-file ${EM_SHELL_FILE}")
    set(emcc_linker_flags "${emcc_linker_flags} -O3 -std=c++17 -s USE_WEBGL2=1 -s GL_UNSAFE_OPTS=1 -s FULL_ES3=1  -s WASM=1 -s MIN_WEBGL_VERSION=1 -s MAX_WEBGL_VERSION=1")  
    set(emcc_linker_flags "${emcc_linker_flags}  --shell-file ${EM_SHELL_FILE}")  
    set(emcc_linker_flags "${emcc_linker_flags} -s ASSERTIONS=1 ")
    
    foreach(asset ${FILE_ASSETS})
        set(emcc_linker_flags "${emcc_linker_flags} --embed-file ${asset}")
    endforeach()

    set_target_properties(${PROJECT_NAME}  
    PROPERTIES SUFFIX ".html"
    LINK_FLAGS ${emcc_linker_flags}
    COMPILE_FLAGS ${emcc_compiler_flags} )
    em_link_js_library(${PROJECT_NAME}  ${libraryJsFiles})
    add_definitions(-D_USE_MATH_DEFINES)
    target_include_directories(${PROJECT_NAME} PRIVATE ${XNLIB_DIR} ${GLM_DIR} ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} )
endmacro()

SDL_Build_EMCC(imgui_sdl ${SRC_DIR}/imgui_sdl.cpp)
SDL_Build_EMCC(main_sim_sdl ${SRC_DIR}/main_sim_sdl.cpp)