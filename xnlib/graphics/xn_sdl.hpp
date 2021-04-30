// #include "xn_common/xn_gui.hpp"

// clang-format off
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include "../external/imgui.h"
#include "../external/imgui_impl_sdl.h"
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#endif


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// clang-format on

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArrays glBindVertexArraysAPPLE
#define glGenVertexArray glGenVertexArrayAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#else
// #include <SDL_opengles2.h>
#endif

#include <iostream>

namespace xn {

namespace sdl {

struct WindowGL {
  SDL_Window *window;
  SDL_GLContext context;
  glm::ivec2 dimensions;

  void (*mouseclick_callback)(const SDL_Event &e) = NULL;
  void (*keypress_callback)(const SDL_Event &e) = NULL;
  void (*default_event_callback)(const SDL_Event &e) = NULL;

  WindowGL() {}

  WindowGL(int w, int h, std::string title = "ImGUI / WASM / WebGL") {
    dimensions = glm::ivec2(w, h);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      std::cerr << "Error: %s\n" << SDL_GetError() << '\n';
      return;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    window = SDL_CreateWindow(title.c_str(),          // title
                              SDL_WINDOWPOS_CENTERED, // x
                              SDL_WINDOWPOS_CENTERED, // y
                              w, h,                   // width, height
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                  SDL_WINDOW_ALLOW_HIGHDPI // flags
    );
    context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

#ifdef __EMSCRIPTEN__
    ImGui_ImplSdl_Init(window);
#else
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      throw -1;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 100");
#endif
  }

  void imguiNewFrame() {
#ifdef __EMSCRIPTEN__
    ImGui_ImplSdl_NewFrame(window);
#else
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
#endif
  }

  void imguiDrawFrame() {
    ImGui::Render();
#ifndef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
  }

  void setMouseClickCallback(void (*callback)(const SDL_Event &e)) {
    mouseclick_callback = callback;
  }

  void setKeyBoardCallback(void (*callback)(const SDL_Event &e)) {
    keypress_callback = callback;
  }

  void setDefaultEventCallback(void (*callback)(const SDL_Event &e)) {
    default_event_callback = callback;
  }

  void setSize(int x, int y) {
    SDL_SetWindowSize(window, x, y);
    dimensions = glm::ivec2(x, y);
  }

  void poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
#ifdef __EMSCRIPTEN__
      ImGui_ImplSdl_ProcessEvent(&event);
#else
      ImGui_ImplSDL2_ProcessEvent(&event);
#endif
      switch (event.type) {
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (mouseclick_callback != NULL)
          mouseclick_callback(event);
        break;

      default:
        if (default_event_callback != NULL)
          default_event_callback(event);
        break;
      }
    }
  }

  void flip() { SDL_GL_SwapWindow(window); }

  static void destroy(WindowGL &window) {
#ifdef __EMSCRIPTEN__
    ImGui_ImplSdl_Shutdown();
#else
    ImGui_ImplSDL2_Shutdown();
#endif
    SDL_GL_DeleteContext(window.context);
    SDL_DestroyWindow(window.window);
    SDL_Quit();
  }
};

void destroySDL(WindowGL &window) {
#ifdef __EMSCRIPTEN__
  ImGui_ImplSdl_Shutdown();
#else
  ImGui_ImplSDL2_Shutdown();
#endif
  SDL_GL_DeleteContext(window.context);
  SDL_DestroyWindow(window.window);
  SDL_Quit();
}

} // namespace sdl

} // namespace xn