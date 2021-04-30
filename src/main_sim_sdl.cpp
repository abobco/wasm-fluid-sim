// Include the Emscripten library only if targetting WebAssembly
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "xn_particles.hpp"
#include <graphics/xn_gl.hpp>

using namespace xn;

sdl::WindowGL window;
json settings;
bool quit = false;
float projection = 1.0f;
float inv_projection = 1.0f;

// frame timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lag = 0.0f;
const float sim_timestep = 1.0 / 60;
const float spawn_speed = 0.01;
uint32_t mouse_state = 0, mouse_btn_2_state;
int PARTICLE_SPAWN_RATE = 8;
bool should_draw_sdf = false;

ParticleSystem particle_system;

std::unordered_map<std::string, Shader> shaders;
ProtoQuad quad;
gl::RenderTexture framebuffer;

// unsigned max_instances = 2047;
unsigned max_instances;
std::vector<glm::vec2> translations;
Box particle_system_bounds{glm::vec2(0, -0.25), glm::vec2(1.0)};

void draw_frame();
void draw_particle_boxes();
void draw_colliders();
void update_gui(ParticleSystem &particle_system, sdl::WindowGL &window);
void load_shaders(std::unordered_map<std::string, Shader> &shaders);

#ifdef __EMSCRIPTEN__
EM_JS(int, canvas_get_width, (),
      { return document.getElementById("canvas").width; });
EM_JS(int, canvas_get_height, (),
      { return document.getElementById("canvas").height; });
#endif

void mouseclick_callback(const SDL_Event &e) {
  if (e.button.button == SDL_BUTTON_LEFT)
    mouse_state = e.button.state;
  if (e.button.button == SDL_BUTTON_RIGHT)
    mouse_btn_2_state = e.button.state;
}

int main(int argc, char *argv[]) {
  printf("%d threads supported by hardware\n",
         std::thread::hardware_concurrency());

  window = sdl::WindowGL(800, 800, "Particle Simulation");
#ifdef __EMSCRIPTEN__
  window.setSize(canvas_get_width(), canvas_get_height());
#endif
  projection = (float)window.dimensions.y / window.dimensions.x;
  inv_projection = 1.0f / projection;

  window.setMouseClickCallback(mouseclick_callback);

  settings = load_json_file("./assets/xn_particle_config.json");
  particle_system = ParticleSystem(settings);
  particle_system.window_bounds = particle_system_bounds;

  framebuffer = gl::RenderTexture(window.dimensions.x, window.dimensions.y);
  quad.init();
  gl::UniformLimits::print();
  auto uniform_limits = gl::UniformLimits::get();

#ifdef __EMSCRIPTEN__
  max_instances = uniform_limits.MAX_VERTEX_UNIFORM_VECTORS;
#else
  max_instances = uniform_limits.MAX_VERTEX_UNIFORM_COMPONENTS / 2;
#endif

  particle_system.PARTICLE_LIMIT = 10000;
  particle_system.particles.reserve(max_instances);
  translations.reserve(max_instances);
  for (int i = 0; i < max_instances; i++) {
    translations.push_back(glm::vec2(0));
  }

  load_shaders(shaders);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(draw_frame, 0, false);
#else

  while (true) {
    draw_frame();
  }
  destroySDL(window);
  return 0;
#endif
}

void draw_frame() {
  uint32_t current_ms = SDL_GetTicks();
  float currentFrame = current_ms / 1000.0f;
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;
  lag += deltaTime;
  while (lag > sim_timestep) {
    particle_system.update();
    lag -= sim_timestep;
  }

  window.poll_events();
  gl::cls(0.2f, 0.3f, 0.3f, 1.0f);
  if (should_draw_sdf) {
    framebuffer.use();
    gl::cls(0.2f, 0.3f, 0.3f, 1.0f);
  }

  // draw container
  {
    glm::mat4 t(1);
    t = glm::scale(t, glm::vec3(projection, 1.0, 1.0));
    t = glm::translate(t,
                       glm::vec3(particle_system.window_bounds.position, 0.0f));
    shaders["basic"].use();
    shaders["basic"].setVec4("color", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }

  if (!should_draw_sdf) {
    draw_particle_boxes();
    draw_colliders();
    window.imguiNewFrame();
    update_gui(particle_system, window);
    window.imguiDrawFrame();
  } else {
    draw_colliders();
    shaders["fluid"].use();
    shaders["fluid"].setInt("numParticles", particle_system.particles.size());
    for (int i = 0; i < particle_system.particles.size(); i++) {
      translations[i] = particle_system.particles[i].position;
      shaders["fluid"].setVec2("translations[" + std::to_string(i) + "]",
                               translations[i]);
    }
    window.imguiNewFrame();
    update_gui(particle_system, window);
    window.imguiDrawFrame();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    framebuffer.draw();
  }

  window.flip();
}

void draw_colliders() {
  shaders["basic"].use();
  shaders["basic"].setVec4("color", glm::vec4(0.3f, 0.3f, 0.8f, 1.0f));
  for (Box &b : particle_system.bodies) {
    glm::mat4 t(1);
    t = glm::scale(t, glm::vec3(projection, 1.0, 1.0));
    t = glm::translate(t, glm::vec3(b.position, 0.0f));
    t = glm::scale(t, glm::vec3(b.dimensions, 1.0f));
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }
}

void draw_particle_boxes() {
  shaders["basic"].use();
  shaders["basic"].setVec4("color", glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
  for (auto &p : particle_system.particles) {
    glm::mat4 t(1);
    t = glm::scale(t, glm::vec3(projection, 1.0, 1.0));
    t = glm::translate(t, glm::vec3(p.position, 0));

    glm::vec3 svec(0.01);
    t = glm::scale(t, svec);
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }
}

void update_gui(ParticleSystem &particle_system, sdl::WindowGL &window) {
  ImGui::Begin("Settings");
  ImGui::Text(
      "Controls:\n  Left click: spawn particle\n  Right click: spawn box "
      "collider");
  ImGui::NewLine();
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text("Particles: %lu", particle_system.particles.size());
#ifdef __EMSCRIPTEN__
  if (ImGui::Button("View on Github")) {
    emscripten_run_script(
        "window.location.href = 'https://github.com/abobco/wasm-fluid-sim';");
  }
#endif
  if (ImGui::Button("Clear Particles")) {
    particle_system.particles.clear();
  }
#ifndef __EMSCRIPTEN__
  ImGui::Checkbox("fluid sdf", &should_draw_sdf);
#endif
  ImGui::SliderInt("spawn_rate", &PARTICLE_SPAWN_RATE, 1, 64);
  ImGui::SliderFloat("particle_radius", &particle_system.r, 0, 0, "%.5f");
  ImGui::SliderFloat("interaction_radius", &particle_system.h, 0, 0, "%.5f");
  ImGui::SliderFloat("stiffness", &particle_system.stiffness, 0, 0.1, "%.8f");
  ImGui::SliderFloat("near_stiffness", &particle_system.near_stiffness, 0, 0.3,
                     "%.8f");

  ImGui::SliderFloat("rest_density", &particle_system.rest_density, 0, 20,
                     "%.5f");
  ImGui::SliderFloat("sigma", &particle_system.sigma, 0, 1, "%.5f");
  ImGui::SliderFloat("beta", &particle_system.beta, 0, 1, "%.5f");
  ImGui::SliderFloat2("gravity", (float *)&particle_system.gravity, -0.01, 0.01,
                      "(%.5f, %.5f)");

  if (!ImGui::GetIO().WantCaptureMouse && (mouse_state || mouse_btn_2_state)) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    glm::vec2 spawn(x, window.dimensions.y - y);
    spawn /= (glm::vec2)window.dimensions;
    spawn *= 2.0;
    spawn += glm::vec2(-1, -1);
    spawn.x *= inv_projection;
    if (mouse_state) {
      if (particle_system.particles.size() + PARTICLE_SPAWN_RATE <
          particle_system.PARTICLE_LIMIT) {
        for (int i = 0; i < PARTICLE_SPAWN_RATE; i++)
          particle_system.addParticle(spawn + glm::vec2(i * 0.005f, i * 0.001f),
                                      glm::vec2(0));
      } else {
        std::rotate(particle_system.particles.begin(),
                    particle_system.particles.begin() + 1,
                    particle_system.particles.end());
        particle_system.particles.back() = Particle(spawn);
      }
    }

    if (mouse_btn_2_state) {
      particle_system.bodies.push_back({spawn, glm::vec2(0.1)});
    }
  }

  ImGui::End();
}

void load_shaders(std::unordered_map<std::string, Shader> &shaders) {
  gl::UniformLimits::print();
  auto uniform_limits = gl::UniformLimits::get();

  std::vector<ShaderLoader> shader_files = {
      {"basic", "basic.vs", "basic.fs"},
      {"screen", "screen.vs", "screen.fs"},
      {"fluid", "screen.vs", "particle_post.fs"}};

  std::vector<std::string> shader_preprocessor_defs = {
      "#define MAX_VERTEX_UNIFORM_VECTORS " + std::to_string(max_instances),
      "#define MAX_FRAGMENT_UNIFORM_VECTORS " +
          std::to_string(uniform_limits.MAX_FRAGMENT_UNIFORM_VECTORS),
      "#define SCREEN_WIDTH " + std::to_string((float)window.dimensions.x),
      "#define SCREEN_HEIGHT " + std::to_string((float)window.dimensions.y)};

  ShaderLoader::load_shader_map(shaders, shader_files,
                                "./assets/shaders/gles300/",
                                shader_preprocessor_defs, "#version 300 es");

  if (!shaders["basic"].success) {
    shaders.clear();
    ShaderLoader::load_shader_map(shaders, shader_files,
                                  "./assets/shaders/gles100/",
                                  shader_preprocessor_defs, "#version 100");
  }
}