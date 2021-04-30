

#ifndef SHADER_H
#define SHADER_H

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#include <glad/glad.h>
#endif

// #endif
// #endif // XN_EGL

#ifdef XN_EGL
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>
#endif // XN_EGL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Shader {
public:
  unsigned int ID;
  std::string vertexCode;
  std::string fragmentCode;
  GLint success = 0;
  // constructor generates the shader on the fly
  // ------------------------------------------------------------------------
  Shader() {}

  Shader(const char *vertexPath, const char *fragmentPath,
         const char *geometryPath = nullptr) {
    // 1. retrieve the vertex/fragment source code from filePath
    // std::string vertexCode;
    // std::string fragmentCode;
    std::string geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;
    // ensure ifstream objects can throw exceptions:
    // vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    // fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    // gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      // open files
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      // std::string x;
      // while (getline(vShaderFile, x)) {
      //   std::cout << x << std::endl;
      // }
      std::stringstream vShaderStream, fShaderStream;
      // read file's buffer contents into streams
      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();
      // close file handlers
      vShaderFile.close();
      fShaderFile.close();
      // convert stream into string
      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();

      // std::cout << "Vertex:\n" << vertexCode << '\n';
      // std::cout << "Fragment:\n" << fragmentCode << '\n';
      // if geometry shader path is present, also load a geometry shader
      if (geometryPath != nullptr) {
        gShaderFile.open(geometryPath);
        std::stringstream gShaderStream;
        gShaderStream << gShaderFile.rdbuf();
        gShaderFile.close();
        geometryCode = gShaderStream.str();
      }
      // } catch (std::ifstream::failure &e) {
    } catch (...) {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

#ifndef XN_EGL
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // if geometry shader is given, compile geometry shader
    // unsigned int geometry;
    // if (geometryPath != nullptr) {
    //     const char *gShaderCode = geometryCode.c_str();
    //     geometry = glCreateShader(GL_GEOMETRY_SHADER_ARB);
    //     glShaderSource(geometry, 1, &gShaderCode, NULL);
    //     glCompileShader(geometry);
    //     checkCompileErrors(geometry, "GEOMETRY");
    // }

    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    // if (geometryPath != nullptr)
    //   glAttachShader(ID, geometry);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer
    // necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    // if (geometryPath != nullptr)
    //   glDeleteShader(geometry);

#endif
  }
  void loadFromText(std::string vertexCode, std::string fragmentCode,
                    std::string geometry = "") {
    this->vertexCode = vertexCode;
    this->fragmentCode = fragmentCode;
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // if geometry shader is given, compile geometry shader
    // unsigned int geometry;
    // if (geometryPath != nullptr) {
    //     const char *gShaderCode = geometryCode.c_str();
    //     geometry = glCreateShader(GL_GEOMETRY_SHADER_ARB);
    //     glShaderSource(geometry, 1, &gShaderCode, NULL);
    //     glCompileShader(geometry);
    //     checkCompileErrors(geometry, "GEOMETRY");
    // }

    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    // if (geometryPath != nullptr)
    //   glAttachShader(ID, geometry);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer
    // necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    // glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
  }
  // activate the shader
  // ------------------------------------------------------------------------
  void use() { glUseProgram(ID); }
  // utility uniform functions
  // ------------------------------------------------------------------------
  void setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
  }
  // ------------------------------------------------------------------------
  void setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
  }
  // ------------------------------------------------------------------------
  void setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
  }
  // ------------------------------------------------------------------------
  void setVec2(const std::string &name, const glm::vec2 &value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
  }
  void setVec2(const std::string &name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
  }
  // ------------------------------------------------------------------------
  void setVec3(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
  }
  void setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
  }
  // ------------------------------------------------------------------------
  void setVec4(const std::string &name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
  }
  void setVec4(const std::string &name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
  }
  // ------------------------------------------------------------------------
  void setMat2(const std::string &name, const glm::mat2 &mat) const {
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                       &mat[0][0]);
  }
  // ------------------------------------------------------------------------
  void setMat3(const std::string &name, const glm::mat3 &mat) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                       &mat[0][0]);
  }
  // ------------------------------------------------------------------------
  void setMat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                       &mat[0][0]);
  }
  // ------------------------------------------------------------------------
  void setMVP(const glm::mat4 &model, const glm::mat4 &view,
              const glm::mat4 &projection) const {
    setMat4("model", model);
    setMat4("view", view);
    setMat4("projection", projection);
  }

  void setTransform(glm::vec3 translation = glm::vec3(0),
                    glm::vec3 scale = glm::vec3(1)) {
    glm::mat4 model(1);
    model = glm::translate(model, translation);
    model = glm::scale(model, scale);
    setMat4("model", model);
  }

private:
  // utility function for checking shader compilation/linking errors.
  // ------------------------------------------------------------------------
  void checkCompileErrors(GLuint shader, std::string type) {
    GLint compileSuccess, linkSuccess;
    // GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
      if (!compileSuccess) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cout
            << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
            << infoLog
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &linkSuccess);
      if (!linkSuccess) {
        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
        std::cout
            << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
            << infoLog
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
      success = linkSuccess;
    }
  }
};

struct ShaderLoader {
  std::string shaderMapKey;
  std::string vertexPath;
  std::string fragmentPath;

  static void
  load_shader_map(std::unordered_map<std::string, Shader> &map,
                  std::vector<ShaderLoader> shader_files,
                  const std::string shader_files_base_path,
                  const std::vector<std::string> shader_preprocessor_defs =
                      std::vector<std::string>(),
                  const std::string glsl_version_def = "#version 300 es") {
    for (auto &s : shader_files) {
      s.vertexPath = shader_files_base_path + s.vertexPath;
      s.fragmentPath = shader_files_base_path + s.fragmentPath;
    }
    for (auto &s : shader_files) {
      std::ifstream vertexFile(s.vertexPath), fragmentFile(s.fragmentPath);
      std::string vertexCode((std::istreambuf_iterator<char>(vertexFile)),
                             std::istreambuf_iterator<char>());
      std::string fragmentCode((std::istreambuf_iterator<char>(fragmentFile)),
                               std::istreambuf_iterator<char>());
      for (const std::string &p : shader_preprocessor_defs) {
        vertexCode = p + "\n" + vertexCode;
        fragmentCode = p + "\n" + fragmentCode;
      }

      vertexCode = glsl_version_def + "\n" + vertexCode;
      fragmentCode = glsl_version_def + "\n" + fragmentCode;

      // std::cout << "Vertex:\n" << vertexCode << "\n";
      // std::cout << "Fragment:\n" << fragmentCode << "\n";

      map[s.shaderMapKey] = Shader();
      map[s.shaderMapKey].loadFromText(vertexCode, fragmentCode);
    }
  }
};

#endif
