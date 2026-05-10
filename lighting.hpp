#pragma once

#include "math.hpp"
#include "scene.hpp"

#include <vector>

double computeLighting(const vec3 &point, const vec3 &normal,
                       const vec3 &cameraDirection,
                       const std::vector<Light> &lights, double specularity);
Light transformLight(const Light &light, const mat4 &cameraMatrix);
