#pragma once

#include "canvas.hpp"
#include "math.hpp"

#include <array>
#include <string>
#include <vector>

struct Material {
  Color color;
  double specularity = -1.0;
};

struct Triangle {
  std::array<int, 3> vertexIndices{};
  int materialIndex = 0;
};

struct Model {
  std::string name;
  std::vector<vec4> vertices;
  std::vector<vec3> vertexNormals;
  std::vector<Triangle> triangles;
  std::vector<Material> materials;
};

void computeVertexNormals(Model &model);
Model loadObj(const std::string &path, Material material);
Model makeCubeModel();
