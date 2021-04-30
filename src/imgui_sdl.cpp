#define XN_SDL

// Include the Emscripten library only if targetting WebAssembly
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <graphics/xn_gl.hpp>
#include <thread>

using namespace xn;

sdl::WindowGL window;
bool quit = false;

// frame timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lag = 0.0f;

float slider_var = 0.5;
uint32_t mouse_state = 0, mouse_btn_2_state;
void draw_frame();

void mouseclick_callback(const SDL_Event &e) {
  if (e.button.button == SDL_BUTTON_LEFT)
    mouse_state = e.button.state;
  if (e.button.button == SDL_BUTTON_RIGHT)
    mouse_btn_2_state = e.button.state;
  DUMP(mouse_state);
  DUMP(e.button.button);
}

int main(int argc, char *argv[]) {
  printf("%d threads supported by hardware\n",
         std::thread::hardware_concurrency());

  window = sdl::WindowGL(800, 800);
  window.setMouseClickCallback(mouseclick_callback);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(draw_frame, 0, false);
#else

  while (true) {
    draw_frame();
  }

  //   gui::cleanup();
  // destroySDL(window);
  return 0;
#endif
}

void draw_frame() {
  gl::cls(0.2f, 0.3f, 0.3f, 1.0f);
  window.poll_events();

  if (mouse_state == SDL_PRESSED) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    printf("mouse click at (%d, %d)\n", x, y);
  }

  // ImGui_ImplSdl_NewFrame(window.window);
  window.imguiNewFrame();
  ImGui::Begin("settings");
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

  ImGui::SliderFloat("Slider", &slider_var, 0.0f, 1.0f);
  ImGui::End();

  window.imguiDrawFrame();
  window.flip();
}
