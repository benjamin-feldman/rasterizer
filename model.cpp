#include "model.hpp"

#include "colors.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr int kRedMaterial = 0;
constexpr int kGreenMaterial = 1;
constexpr int kBlueMaterial = 2;
constexpr int kYellowMaterial = 3;
constexpr int kPurpleMaterial = 4;
constexpr int kCyanMaterial = 5;
}

void computeVertexNormals(Model &model) {
  model.vertexNormals.assign(model.vertices.size(), vec3{0.0, 0.0, 0.0});

  const int vertexCount = static_cast<int>(model.vertices.size());
  for (const Triangle &triangle : model.triangles) {
    assert(triangle.vertexIndices[0] >= 0 &&
           triangle.vertexIndices[0] < vertexCount);
    assert(triangle.vertexIndices[1] >= 0 &&
           triangle.vertexIndices[1] < vertexCount);
    assert(triangle.vertexIndices[2] >= 0 &&
           triangle.vertexIndices[2] < vertexCount);

    const vec3 v0 = model.vertices[triangle.vertexIndices[0]].xyz();
    const vec3 v1 = model.vertices[triangle.vertexIndices[1]].xyz();
    const vec3 v2 = model.vertices[triangle.vertexIndices[2]].xyz();
    const vec3 faceNormal = normalizeOrZero(cross(v1 - v0, v2 - v0));

    model.vertexNormals[triangle.vertexIndices[0]] =
        model.vertexNormals[triangle.vertexIndices[0]] + faceNormal;
    model.vertexNormals[triangle.vertexIndices[1]] =
        model.vertexNormals[triangle.vertexIndices[1]] + faceNormal;
    model.vertexNormals[triangle.vertexIndices[2]] =
        model.vertexNormals[triangle.vertexIndices[2]] + faceNormal;
  }

  for (vec3 &normal : model.vertexNormals) {
    normal = normalizeOrZero(normal);
  }
}

Model loadObj(const std::string &path, Material material) {
  Model model;
  model.name = std::filesystem::path(path).filename().string();
  model.materials.push_back(material);

  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("could not open OBJ file: " + path);
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream stream(line);
    std::string token;
    stream >> token;

    if (token == "v") {
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      stream >> x >> y >> z;
      model.vertices.push_back({x, y, z, 1.0});
    } else if (token == "f") {
      std::vector<int> face;
      std::string vertex;
      while (stream >> vertex) {
        int index =
            std::stoi(vertex); // Handles v/vt/vn because stoi stops at '/'.
        if (index < 0) {
          index = static_cast<int>(model.vertices.size()) + index + 1;
        }
        face.push_back(index - 1); // OBJ indices are 1-based.
      }

      for (int i = 1; i + 1 < static_cast<int>(face.size()); i++) {
        model.triangles.push_back(Triangle{{face[0], face[i], face[i + 1]}, 0});
      }
    }
  }

  computeVertexNormals(model);
  return model;
}

Model makeCubeModel() {
  Model cube;
  cube.name = "cube";
  cube.vertices = {
      {1.0, 1.0, 1.0, 1.0},    {-1.0, 1.0, 1.0, 1.0},  {-1.0, -1.0, 1.0, 1.0},
      {1.0, -1.0, 1.0, 1.0},   {1.0, 1.0, -1.0, 1.0},  {-1.0, 1.0, -1.0, 1.0},
      {-1.0, -1.0, -1.0, 1.0}, {1.0, -1.0, -1.0, 1.0},
  };

  // The cube uses default material specularity on each face.
  cube.materials = {
      Material{colors::red},    Material{colors::green},
      Material{colors::blue},   Material{colors::yellow},
      Material{colors::purple}, Material{colors::cyan},
  };
  cube.triangles = {
      Triangle{{0, 1, 2}, kRedMaterial},
      Triangle{{0, 2, 3}, kRedMaterial},
      Triangle{{4, 0, 3}, kGreenMaterial},
      Triangle{{4, 3, 7}, kGreenMaterial},
      Triangle{{5, 4, 7}, kBlueMaterial},
      Triangle{{5, 7, 6}, kBlueMaterial},
      Triangle{{1, 5, 6}, kYellowMaterial},
      Triangle{{1, 6, 2}, kYellowMaterial},
      Triangle{{4, 5, 1}, kPurpleMaterial},
      Triangle{{4, 1, 0}, kPurpleMaterial},
      Triangle{{2, 6, 7}, kCyanMaterial},
      Triangle{{2, 7, 3}, kCyanMaterial},
  };

  computeVertexNormals(cube);
  return cube;
}
