#pragma once

#include <graphics/xn_gl.hpp>
// #include <graphics/xn_gui.hpp>
#include <string.h>
#include <thread>
#include <util/xn_json.hpp>

#include <algorithm>
#include <array>
#include <execution>
#include <glm/gtx/norm.hpp>
#include <unordered_map>
#include <vector>

namespace xn {

// if t > 0:     return 1
// if t < 0:     return -1
// if t == 0:    return 0
template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

template <class InputIt, class UnaryFunction>
void xn_for_each(InputIt first, InputIt last, UnaryFunction f) {
#ifndef __EMSCRIPTEN__
  // unsequenced for loops are faster
  std::for_each(std::execution::par, first, last, f);
#else
  std::for_each(first, last, f);
#endif
}

struct ProtoQuad {
  float *verts;
  unsigned *indices;
  unsigned EBO;
  unsigned VBO;
  unsigned VAO;

  ProtoQuad() {}
  void init() {
    float def_vertices[] = {
        0.5f,  0.5f,  0.0f, // top right
        0.5f,  -0.5f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f,  0.0f  // top left
    };
    unsigned int def_indices[] = {
        // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    verts = new float[sizeof(def_vertices) / sizeof(float)];
    indices = new unsigned[sizeof(def_indices) / sizeof(unsigned)];

    std::memcpy(verts, def_vertices, sizeof(def_vertices));
    std::memcpy(indices, def_indices, sizeof(def_indices));

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(def_vertices), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(def_indices), indices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
  }

  void draw(Shader &shader) {
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
};

struct Particle {
  bool isNew = true;
  float r = 0.01;
  glm::vec2 position{0, 0};
  glm::vec2 f{0, 0};
  int ticks = 60;
  glm::vec2 o;
  glm::vec2 velocity;
  float density = 0;
  float near_density = 0;

  Particle(glm::vec2 spawn = glm::vec2(0, 0),
           glm::vec2 velocity = glm::vec2(0, 0), float r = 0.01) {
    this->position = spawn;
    this->velocity = velocity;
    this->o = spawn;
    this->r = r;
  }

  void draw(ProtoQuad quad, Shader &shader) {
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(position.x, position.y, 0.0f));
    trans = glm::scale(trans, glm::vec3(0.05f, 0.05f, 0.05f));
    shader.setMat4("transform", trans);
    quad.draw(shader);
  }
};

struct GridHash {
  std::size_t operator()(const glm::ivec2 &node) const {
    std::size_t h1 = std::hash<int>()(node.x);
    std::size_t h2 = std::hash<int>()(node.y);
    return h1 ^ h2;
  }
};

struct SpatialHash {
  float h;
  std::unordered_map<glm::ivec2, std::vector<Particle *>, GridHash> grid;

  SpatialHash() {}

  SpatialHash(float h) { this->h = h; }

  glm::ivec2 worldToGrid(glm::vec2 position) {
    return glm::ivec2(position.x / this->h, position.y / this->h);
  }

  void update(std::vector<Particle> &particles) {
    for (auto &pair : grid) {
      pair.second.clear();
    }
    // grid.clear();

    for (auto &p : particles) {
      glm::ivec2 gp = worldToGrid(p.position);
      if (grid.find(gp) == grid.end()) {
        grid[gp] = std::vector<Particle *>();
        // grid[gp].reserve(1024);
      }
      grid[gp].push_back(&p);
    }
  }

  std::vector<Particle *> getNeighbors(Particle p) {
    glm::ivec2 bbmin = worldToGrid(p.position - h);
    glm::ivec2 bbmax = worldToGrid(p.position + h);
    std::mutex near_particles_lock;
    std::vector<Particle *> near_particles;

    for (int x = bbmin.x; x <= bbmax.x; x++) {
      for (int y = bbmin.y; y <= bbmax.y; y++) {
        glm::ivec2 gridkey(x, y);
        if (grid.find(gridkey) == grid.end()) {
          continue;
        }

        std::vector<Particle *> &cell_particles = grid[gridkey];
        // cell_particles.reserve(1024);
        for (auto &cp : cell_particles) {
          float d = glm::length2(p.position - cp->position);
          if (d < h * h) {
            near_particles.push_back(cp);
          }
        }
      }
    }
    return near_particles;
  }
};

struct Box {
  glm::vec2 position;
  glm::vec2 dimensions;
};

struct ParticleSystem {
  std::vector<Particle> particles;
  std::vector<std::vector<Particle *>> near_particles_cache;
  std::vector<Box> bodies;
  Box window_bounds{glm::vec2(0, 0), glm::vec2(1, 1)};

  glm::vec2 gravity;

  float r;
  float h;

  float stiffness;
  float near_stiffness;
  float rest_density;

  float sigma;
  float beta;

  unsigned PARTICLE_LIMIT = 2048;
  SpatialHash hash;

  ParticleSystem() {}

  ParticleSystem(const json &settings) {
    gravity =
        glm::vec2((float)settings["gravity"][0], (float)settings["gravity"][1]);
    r = (float)settings["particle_radius"];
    h = (float)settings["interaction_radius"];
    stiffness = (float)settings["stiffness"];
    near_stiffness = (float)settings["near_stiffness"];
    rest_density = (float)settings["rest_density"];
    sigma = (float)settings["sigma"];
    beta = (float)settings["beta"];

    hash = SpatialHash(h);
  }

  void setParticleLimit(unsigned limit) {
    PARTICLE_LIMIT = limit;
    particles.reserve(limit);
  }

  void viscosityImpulses(int i, float delta = 1.0f) {
    Particle &iParticle = particles[i];
    near_particles_cache[i] = hash.getNeighbors(iParticle);
    std::vector<Particle *> &near_particles = near_particles_cache[i];

    for (Particle *jParticle : near_particles) {
      glm::vec2 dp = iParticle.position - jParticle->position;
      float r2 = glm::dot(dp, dp);

      if (r2 <= 0.0 || r2 > h * h)
        continue;

      float r = sqrt(r2);
      glm::vec2 r_norm = glm::normalize(dp);
      float one_minus_q = 1.0 - r / h;
      glm::vec2 vi_minus_vj = iParticle.velocity - jParticle->velocity;
      float u = glm::dot(vi_minus_vj, r_norm);

      float t = 0;
      if (u > 0.0f) {
        t = delta * one_minus_q * (sigma * u + beta * u * u) * 0.5f;
        if (t > u)
          t = u;
      } else {
        t = delta * one_minus_q * (sigma * u - beta * u * u) * 0.5;
        if (t < u)
          t = u;
      }

      glm::vec2 i_div2 = r_norm * t;

      iParticle.velocity += (i_div2 * -1.0f);
      jParticle->velocity += (i_div2 * 1.0f);
    }
  }

  void computeDensities(int i) {
    std::vector<Particle *> &near_particles = near_particles_cache[i];
    Particle &iParticle = particles[i];

    for (auto &jParticle : near_particles) {
      glm::vec2 dp = iParticle.position - jParticle->position;
      float r2 = glm::dot(dp, dp);
      if (r2 <= 0.0f || r2 > h * h)
        continue;

      float r = sqrt(r2);
      float a = 1.0f - r / h;
      float aa = a * a;
      float aaa = aa * a;

      iParticle.density += aa;
      jParticle->density += aa;

      iParticle.near_density += aaa;
      jParticle->near_density += aaa;
    }
  }

  void doubleDensityRelaxation(float delta) {
    std::vector<int> idx_vec;
    idx_vec.reserve(particles.size());
    for (int i = 0; i < particles.size(); i++) {
      idx_vec.push_back(i);
    }
    xn_for_each(idx_vec.begin(), idx_vec.end(), [&](int i) {
      Particle &iParticle = particles[i];
      std::vector<Particle *> &near_particles = near_particles_cache[i];
      float pressure = stiffness * (iParticle.density - rest_density);
      float near_pressure = near_stiffness * iParticle.near_density;

      xn_for_each(near_particles.begin(), near_particles.end(),
                  [&](Particle *jParticle) {
                    glm::vec2 dp = iParticle.position - jParticle->position;
                    float r2 = glm::dot(dp, dp);

                    if (r2 <= 0.0f || r2 > h * h)
                      return;

                    float r = sqrt(r2);
                    float a = 1.0f - r / h;
                    float d = delta * delta *
                              (pressure * a + near_pressure * a * a) * 0.5;
                    glm::vec2 da = dp * (d / r);

                    iParticle.f += da * 1.0f;
                    jParticle->f += da * -1.0f;
                  });
    });
  }

  void boxCollision(Particle &p, Box &b, bool is_container = false) {
    // either add or subract particle radius from signed distance
    // depending on if the box is a container or not
    int sign = -1;
    if (is_container)
      sign = 1;

    // convert particle position into local box coords
    glm::vec2 &x = p.position;
    glm::vec2 &c = b.position;
    glm::vec2 xLocal = x - c;

    // map particle position to quadrant 1 of box axes
    glm::vec2 xAbs(fabsf(xLocal.x), fabsf(xLocal.y));
    glm::vec2 br = b.dimensions * 0.5f;
    glm::vec2 d = xAbs - br; // distance vector mapped to quadrant 1

    // signed distance between box and circle
    float sd = glm::length(glm::vec2(std::max(d.x, 0.0f), std::max(d.y, 0.0f)));
    sd += std::min(std::max(d.x, d.y), 0.0f);
    sd += this->r * sign;

    // collision response
    if (is_container && sd >= 0 || !is_container && sd <= 0) {
      // compute local contact point
      glm::vec2 cp_local(std::min(br.x, std::max(-br.x, xLocal.x)),
                         std::min(br.y, std::max(-br.y, xLocal.y)));

      // surface normal
      glm::vec2 n(sgn(cp_local.x - xLocal.x), sgn(cp_local.y - xLocal.y));

      if (glm::length2(n) == 0.0f)
        return;
      n = glm::normalize(n);

      // decompose velocity into normal/tangent velocity relative to the
      // surface
      glm::vec2 vel_tangent = n * glm::dot(p.velocity, n);
      glm::vec2 vel_normal = p.velocity - vel_tangent;

      // apply friction to tangent velocity
      vel_tangent *= 0.5f;

      // remove normal velocity
      p.velocity = vel_tangent;

      // add impulse for collision
      p.f -= (vel_normal - (vel_tangent * 0.01f));
      // p.f -= vel_normal;

      // apply translation to particle
      if (!is_container) {
        p.position += (n * sd);
        // p.position = c + cp_local - n * 0.01f;
        // p.position = c + cp_local + (n * sd);
      } else {
        p.position = c + cp_local;
      }
    }
  }

  void handleCollision(Particle &p) {
    for (auto &b : bodies) {
      boxCollision(p, b);
    }

    boxCollision(p, window_bounds, true);
  }

  void addParticle(glm::vec2 position, glm::vec2 velocity) {
    particles.push_back(Particle(position, velocity, r));
  }

  void update() {
    near_particles_cache.clear();
    hash.update(particles);

    std::vector<unsigned> idx_range;
    idx_range.reserve(particles.size());
    for (auto i = 0; i < particles.size(); i++) {
      idx_range.push_back(i);
      near_particles_cache.push_back(std::vector<Particle *>());
    }

    xn_for_each(std::begin(idx_range), std::end(idx_range), [&](unsigned i) {
      Particle &p = particles[i];
      p.position += p.f;
      p.f = glm::vec2(0);
      p.near_density = 0;
      p.density = 0;
      if (!p.isNew) {
        p.velocity = p.position - p.o;
      }
      p.isNew = false;
      p.velocity += gravity;
      viscosityImpulses(i);
    });
    xn_for_each(std::begin(idx_range), std::end(idx_range), [&](unsigned i) {
      Particle &p = particles[i];
      p.o = p.position;
      p.position += p.velocity;
      computeDensities(i);
      handleCollision(p);
    });
    doubleDensityRelaxation(0.4);
  }
};
} // namespace xn