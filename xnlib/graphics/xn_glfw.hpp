
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <string>

namespace xn {
namespace glfw {
GLFWwindow *CreateWindow(int width, int height, std::string name) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  GLFWwindow *window =
      glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    throw -1;
  }
  glfwMakeContextCurrent(window);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    throw -1;
  }

  return window;
}
struct Window {
  GLFWwindow *handle;
  glm::ivec2 dimensions;

  Window() {}

  Window(int width, int height, std::string name) {
    dimensions = glm::ivec2(width, height);
    handle = glfw::CreateWindow(width, height, name);
  }
  glm::ivec2 getSize() {
    glm::ivec2 s;
    glfwGetWindowSize(handle, &s.x, &s.y);
    return s;
  }

  void resize(int x, int y) {
    glfwSetWindowSize(handle, x, y);
    glViewport(0, 0, x, y);
  }

  void close() { glfwSetWindowShouldClose(handle, true); }
  bool shouldClose() { return glfwWindowShouldClose(handle); }

  glm::dvec2 getCursorPos() {
    glm::dvec2 c;
    glfwGetCursorPos(handle, &c.x, &c.y);
    return c;
  }

  int getKey(int key) { return glfwGetKey(handle, key); }

  void flip() { glfwSwapBuffers(handle); }
};
} // namespace glfw
} // namespace xn