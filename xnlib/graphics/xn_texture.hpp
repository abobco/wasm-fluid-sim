#include "../external/stb_image.h"
// #include "glad/glad.h"
// #include <GLFW/glfw3.h>
#include <iostream>

namespace xn {
namespace gl {
struct Texture2D {
  int wrap_mode_s;
  int wrap_mode_t;
  int filter_mode;
  int mipmap_mode;
  int width;
  int height;
  int channel_count;
  unsigned id;
  int uniform_idx;

  Texture2D() {}

  Texture2D(const char *filename, int uniform_idx = GL_TEXTURE0,
            bool flip = true) {
    setTexParams(uniform_idx);

    stbi_set_flip_vertically_on_load(flip);
    unsigned char *data =
        stbi_load(filename, &width, &height, &channel_count, 0);

    if (data) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    } else {
      std::cout << "texture load failed for " << filename << '\n';
    }

    stbi_image_free(data);
  }

  Texture2D(unsigned char *image_data, int len, int uniform_idx = GL_TEXTURE0,
            bool flip = true) {
    setTexParams(uniform_idx);
    stbi_set_flip_vertically_on_load(flip);
    unsigned char *data = stbi_load_from_memory(image_data, len, &width,
                                                &height, &channel_count, 0);

    if (data) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    } else {
      std::cout << "texture load failed from memory\n";
    }

    stbi_image_free(data);
  }

  void setTexParams(int uniform_idx) {
    this->uniform_idx = uniform_idx;
    glGenTextures(1, &id);
    glActiveTexture(uniform_idx);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  void activate() {
    glActiveTexture(uniform_idx);
    glBindTexture(GL_TEXTURE_2D, id);
  }
};

} // namespace gl
} // namespace xn