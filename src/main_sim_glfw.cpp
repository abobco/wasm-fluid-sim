// Old code, replaced with main_sim_sdl.cpp
// Left here in case a glfw version of the app is ever needed

#define PIO_VIRTUAL
#define HAVE_STRUCT_TIMESPEC
#define GLM_ENABLE_EXPERIMENTAL
#define _USE_MATH_DEFINES

#define XN_LIT true
// Include the Emscripten library only if targetting WebAssembly
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#endif

// clang-format off
#include <graphics/xn_gui.hpp>
#include "xn_particles.hpp"
// clang-format on
using namespace xn;

// frame timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lag = 0.0f;
const float sim_timestep = 1.0 / 60;
const float spawn_speed = 0.01;
int mouse_state = 0, mouse_btn_2_state;
int PARTICLE_SPAWN_RATE = 16;
bool should_draw_sdf = false;

glfw::Window window;
json settings;

bool orth = true, orbit = true, mouse_held = false;
ParticleSystem particle_system;

std::unordered_map<std::string, Shader> shaders;
ProtoQuad quad;
gl::RenderTexture framebuffer;

// unsigned max_instances = 2047;
unsigned max_instances;
std::vector<glm::vec2> translations;
Box particle_system_bounds{glm::vec2(0, -0.25), glm::vec2(1.0)};

void processInput(glfw::Window &window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouseclick_callback(GLFWwindow *window, int button, int action, int mods);
void update_sim();
void draw_particle_boxes();
void update_gui(ParticleSystem &, glfw::Window &);
void hash_to_texture(const SpatialHash &hash, gl::RenderTexture &texture);
static void error_callback(int error, const char *description);

int main(int argc, char *argv[]) {
  // Setup the Error handler
  glfwSetErrorCallback(error_callback);
  printf("%d threads supported by hardware\n",
         std::thread::hardware_concurrency());

  // init opengl
  window = glfw::Window(800, 800, "boxin");
  glfwSetMouseButtonCallback(window.handle, mouseclick_callback);
  glfwSetFramebufferSizeCallback(window.handle, framebuffer_size_callback);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glfwSwapInterval(1);

  quad.init();
  framebuffer = gl::RenderTexture(window.dimensions.x, window.dimensions.y);

  settings = load_json_file("assets/xn_particle_config.json");
  particle_system = ParticleSystem(settings);
  particle_system.window_bounds = particle_system_bounds;

  // Setup Dear ImGui context
  gui::init(window.handle);

  DUMP(glfwGetVersionString());
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

  std::vector<ShaderLoader> shader_files = {
      {"basic", "basic.vs", "basic.fs"},
      {"instanced", "instanced.vs", "instanced.fs"},
      {"screen", "screen.vs", "screen.fs"},
      {"fluid", "screen.vs", "particle_post.fs"},
  };

  std::vector<std::string> shader_preprocessor_defs = {
      "#define MAX_VERTEX_UNIFORM_VECTORS " + std::to_string(max_instances),
      "#define MAX_FRAGMENT_UNIFORM_VECTORS " +
          std::to_string(uniform_limits.MAX_FRAGMENT_UNIFORM_VECTORS)};

  ShaderLoader::load_shader_map(shaders, shader_files, "assets/shaders/",
                                shader_preprocessor_defs);

  shaders["instanced"].use();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(update_sim, 0, false);
#else

  while (!window.shouldClose()) {
    update_sim();
  }

  gui::cleanup();
  return 0;
#endif
}

void update_sim() {
  float currentFrame = (float)glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;
  lag += deltaTime;

  glfwPollEvents();
  gl::cls(0.2f, 0.3f, 0.3f, 1.0f);

  while (lag > sim_timestep) {
    particle_system.update();
    lag -= sim_timestep;
  }
#ifndef __EMSCRIPTEN__
  framebuffer.use();
#endif
  // draw box colliders
  shaders["basic"].use();
  shaders["basic"].setVec4("color", glm::vec4(0.3f, 0.3f, 0.8f, 1.0f));
  for (Box &b : particle_system.bodies) {
    glm::mat4 t(1);
    t = glm::translate(t, glm::vec3(b.position, 0.0f));
    t = glm::scale(t, glm::vec3(b.dimensions, 1.0f));
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }
  if (!should_draw_sdf)
    draw_particle_boxes();

  // draw container
  {
    glm::mat4 t(1);
    t = glm::translate(t,
                       glm::vec3(particle_system.window_bounds.position, 0.0f));
    shaders["basic"].use();
    shaders["basic"].setVec4("color", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }

#ifndef __EMSCRIPTEN__
  if (should_draw_sdf) {
    shaders["fluid"].use();
    shaders["fluid"].setInt("numParticles", particle_system.particles.size());
    for (int i = 0; i < particle_system.particles.size(); i++) {
      translations[i] = particle_system.particles[i].position;
      shaders["fluid"].setVec2("translations[" + std::to_string(i) + "]",
                               translations[i]);
    }
  } else {
    shaders["screen"].use();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  framebuffer.draw();
#endif

  update_gui(particle_system, window);
  gui::draw();

  // swap framebuffers
  window.flip();
}

void update_gui(ParticleSystem &particle_system, glfw::Window &window) {
  gui::update_frame();
  ImGui::Begin("Settings");
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text("Particles: %d", particle_system.particles.size());
  if (ImGui::Button("Clear Particles")) {
    particle_system.particles.clear();
  }
  ImGui::Checkbox("fluid sdf", &should_draw_sdf);
  ImGui::SliderFloat("particle_radius", &particle_system.r, 0.0, 0.2, "%.5f",
                     0);
  ImGui::SliderFloat("interaction_radius", &particle_system.h, 0.0, 0.2, "%.5f",
                     0);
  ImGui::SliderFloat("stiffness", &particle_system.stiffness, 0.0, 0.1, "%.8f",
                     0);
  ImGui::SliderFloat("near_stiffness", &particle_system.near_stiffness, 0.0,
                     0.3, "%.8f", 0);

  ImGui::SliderFloat("rest_density", &particle_system.rest_density, 0.0, 20.0,
                     "%.5f", 0);
  ImGui::SliderFloat("sigma", &particle_system.sigma, 0.0, 1.0, "%.5f", 0);
  ImGui::SliderFloat("beta", &particle_system.beta, 0.0, 1.0, "%.5f", 0);
  ImGui::SliderFloat2("gravity", (float *)&particle_system.gravity, -0.01, 0.01,
                      "(%.5f, %.5f)", 0);

  if (!ImGui::GetIO().WantCaptureMouse && (mouse_state || mouse_btn_2_state)) {
    double x, y;
    glfwGetCursorPos(window.handle, &x, &y);
    glm::vec2 spawn(x, window.dimensions.y - y);
    spawn /= (glm::vec2)window.dimensions;
    spawn *= 2.0;
    spawn += glm::vec2(-1, -1);
    if (mouse_state) {
      // particle_system.addParticle(spawn, glm::vec2(0));
      // particle_system.
      if (particle_system.particles.size() + PARTICLE_SPAWN_RATE <
          particle_system.PARTICLE_LIMIT) {
        for (int i = 0; i < PARTICLE_SPAWN_RATE; i++)
          particle_system.addParticle(spawn + glm::vec2(i * 0.005f, 0),
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

void draw_particle_boxes() {
#ifdef __EMSCRIPTEN__
  // glDrawElementsInstanced not implemented, so draw each particle
  // individually
  shaders["basic"].use();
  shaders["basic"].setVec4("color", glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
  for (auto &p : particle_system.particles) {
    glm::mat4 t(1);
    t = glm::translate(t, glm::vec3(p.position, 0));

    t = glm::scale(t, glm::vec3(0.01));
    shaders["basic"].setMat4("transform", t);
    quad.draw(shaders["basic"]);
  }
#else
  // draw particles in batches
  const int p_count = particle_system.particles.size();
  for (int bi = 0; bi < p_count; bi += max_instances) {
    shaders["instanced"].use();
    shaders["instanced"].setFloat("particle_radius", particle_system.r);

    const int max_i =
        (bi + max_instances) > p_count ? p_count : bi + max_instances;
    for (int i = bi; i < max_i; i++) {
      const int ti = i - bi;
      translations[ti] = particle_system.particles[i].position;
      shaders["instanced"].setVec2("translations[" + std::to_string(ti) + "]",
                                   translations[ti]);
    }
    glBindVertexArray(quad.VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                            particle_system.particles.size());
  }
#endif
}

// Handle GLFW Errors
static void error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s\n", description);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  gl::win_size_pixels = glm::ivec2(width, height);
}

void mouseclick_callback(GLFWwindow *window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_1)
    mouse_state = action;
  else if (button == GLFW_MOUSE_BUTTON_2)
    mouse_btn_2_state = action;
}

// void load_shader_map(std::unordered_map<std::string, Shader> &map,
//                      std::vector<ShaderLoadInfo> shader_files,
//                      const std::string shader_files_base_path,
//                      const std::vector<std::string> shader_preprocessor_defs,
//                      const std::string glsl_version_def) {
//   for (auto &s : shader_files) {
//     s.vertexPath = shader_files_base_path + s.vertexPath;
//     s.fragmentPath = shader_files_base_path + s.fragmentPath;
//   }
//   for (auto &s : shader_files) {
//     std::ifstream vertexFile(s.vertexPath), fragmentFile(s.fragmentPath);
//     std::string vertexCode((std::istreambuf_iterator<char>(vertexFile)),
//                            std::istreambuf_iterator<char>());
//     std::string fragmentCode((std::istreambuf_iterator<char>(fragmentFile)),
//                              std::istreambuf_iterator<char>());
//     for (const std::string &p : shader_preprocessor_defs) {
//       vertexCode = p + "\n" + vertexCode;
//       fragmentCode = p + "\n" + fragmentCode;
//     }

//     vertexCode = glsl_version_def + "\n" + vertexCode;
//     fragmentCode = glsl_version_def + "\n" + fragmentCode;

//     map[s.shaderMapKey] = Shader();
//     map[s.shaderMapKey].loadFromText(vertexCode, fragmentCode);
//   }
// }

void hash_to_texture(const SpatialHash &hash, gl::RenderTexture &texture) {
  /**
   * Spatial hash: divides space into a sparse grid containing disjoint sets of
   * particles
   *
   * The fluid sdf fragment shader needs to perform a union on all particles
   * that are near the active fragment, which can be very slow if iterating over
   * a large group of particles
   *
   * Accessing this spatial hash from the fragment shader could reduce the
   * number of union ops
   *
   * Options:
   *
   * - Implement a hashmap in GLSL:
   *
   *    1. store the particle positions in a vec2 uniform array,
   *    2. store the number of particles in each grid square in an integer
   *       uniform array,
   *    3. implement 2 hashing functions in GLSL:
   *        3.1 TexCoords => index of first particle in the sublist of particles
   *                         in the same grid square as TexCoords
   *        3.2 TexCoords => index of the number of particles in the same grid
   *                         square as TexCoords
   *    4. Use these hashing functions to get the pruned list of near particles
   *
   *     * Pros:
   *        - does not use any texture memory
   *        - similar number of uniform attachments as the naive method
   *     * Cons:
   *        - Limits number of particles to (GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
   * - (number of grid squares) ) Mobile WebGL platforms have much smaller
   * limits on this than desktop On my dev laptop, the limit would still be
   * around 2000 particles, which is not that much
   *        - unclear how to implement the hashing function described in 3.1
   *
   * - Store the hashmap in a 2d texture:
   *    1. Create a black texture the same aspect ratio as the fluid domain
   *    2. Divide texture into grid squares of same size ratio as the spatial
   * hash grid 3.
   *
   *
   * */
}