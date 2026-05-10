#pragma once

#include "math.hpp"
#include "model.hpp"
#include "scene.hpp"

#include <vector>

struct Plane {
  vec3 normal;
  double d;
};

struct ClipTriangle {
  // Ad hoc triangle used while model geometry moves through clipping.
  vec4 v0;
  vec4 v1;
  vec4 v2;
  vec3 n0;
  vec3 n1;
  vec3 n2;
  Material material;
};

struct ClippedInstance {
  std::vector<ClipTriangle> triangles;
};

std::vector<ClippedInstance> clipScene(const Scene &scene,
                                       const std::vector<Plane> &planes,
                                       const mat4 &cameraMatrix);
