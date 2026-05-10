#pragma once

#include "math.hpp"
#include "model.hpp"

#include <vector>

enum class LightType {
  Point,
  Directional,
  Ambient,
};

struct Light {
  LightType type = LightType::Ambient;
  vec3 position{};
  vec3 direction{};
  double intensity = 0.0;
};

struct ModelInstance {
  const Model *model = nullptr;
  mat4 transform{};
};

struct Scene {
  std::vector<ModelInstance> instances;
  std::vector<Light> lights;
};

struct Camera {
  vec3 position;
  vec3 target;
};
