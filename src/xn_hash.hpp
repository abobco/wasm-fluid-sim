#pragma once

#include "xn_particles.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace xn {
struct SpatialHash {
  float h;
  std::unordered_map<glm::ivec2, std::vector<Particle>> grid;

  SpatialHash(float h) { this->h = h; }

  glm::ivec2 worldToGrid(glm::vec2 position) {
    return glm::ivec2(position.x / this->h, position.y / this->h);
  }

  void update(std::vector<Particle> particles) {
    // for (auto &pair : grid) {
    //   pair.second.clear();
    // }
    grid.clear();

    for (auto &p : particles) {
      glm::ivec2 gp = worldToGrid(p.position);
      if (grid.find(gp) == grid.end())
        grid[gp] = std::vector<Particle>();
      grid[gp].push_back(p);
    }
  }

  std::vector<Particle> getNeighbors(Particle p) {
    glm::ivec2 bbmin = worldToGrid(p.position - h);
    glm::ivec2 bbmax = worldToGrid(p.position + h);
    std::vector<Particle> near_particles;

    for (int x = bbmin.x; x <= bbmax.x; x++) {
      for (int y = bbmin.y; y <= bbmax.y; y++) {
        glm::ivec2 gridkey(x, y);
        if (grid.find(gridkey) == grid.end()) {
          continue;
        }

        std::vector<Particle> &cell_particles = grid[gridkey];
        for (auto &cp : cell_particles) {
          float d = glm::length2(p.position - cp.position);
          if (d < h * h) {
            near_particles.push_back(cp);
          }
        }
      }
    }
    return near_particles;
  }
};
} // namespace xn