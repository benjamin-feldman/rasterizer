#include "renderer.hpp"

#include "clipping.hpp"
#include "lighting.hpp"
#include "rasterizer.hpp"

#include <vector>

namespace {
void renderClippedInstance(Canvas &canvas, const ClippedInstance &instance,
                           const mat34 &projectionMatrix,
                           const std::vector<Light> &lights) {
  for (const ClipTriangle &triangle : instance.triangles) {
    // Backface culling.
    vec3 faceNormal = cross(triangle.v1.xyz() - triangle.v0.xyz(),
                            triangle.v2.xyz() - triangle.v0.xyz());
    if (norm2(faceNormal) == 0.0)
      continue;

    faceNormal = normalize(faceNormal);
    if (dot(faceNormal, triangle.v0.xyz()) >= 0.0)
      continue;

    const vec3 projected0 = projectionMatrix * triangle.v0;
    const vec3 projected1 = projectionMatrix * triangle.v1;
    const vec3 projected2 = projectionMatrix * triangle.v2;

    const vec2 canvasVertex0 = {projected0.x / projected0.z,
                                projected0.y / projected0.z};
    const vec2 canvasVertex1 = {projected1.x / projected1.z,
                                projected1.y / projected1.z};
    const vec2 canvasVertex2 = {projected2.x / projected2.z,
                                projected2.y / projected2.z};

    // computeLighting expects: point position, surface normal, direction from
    // point to camera, scene lights, and material specularity. Vertices are in
    // camera space, so the camera is at the origin and point->camera is -point.
    const double illumination0 = computeLighting(
        triangle.v0.xyz(), triangle.n0, -1.0 * triangle.v0.xyz(), lights,
        triangle.material.specularity);
    const double illumination1 = computeLighting(
        triangle.v1.xyz(), triangle.n1, -1.0 * triangle.v1.xyz(), lights,
        triangle.material.specularity);
    const double illumination2 = computeLighting(
        triangle.v2.xyz(), triangle.n2, -1.0 * triangle.v2.xyz(), lights,
        triangle.material.specularity);

    const ScreenVertex screenVertex0{canvasVertex0,
                                     {illumination0, 1.0 / projected0.z}};
    const ScreenVertex screenVertex1{canvasVertex1,
                                     {illumination1, 1.0 / projected1.z}};
    const ScreenVertex screenVertex2{canvasVertex2,
                                     {illumination2, 1.0 / projected2.z}};

    drawShadedTriangle(canvas, screenVertex0, screenVertex1, screenVertex2,
                       triangle.material.color);
  }
}
} // namespace

void renderScene(Canvas &canvas, const Viewport &viewport, const Camera &camera,
                 const Scene &scene) {
  const mat4 cameraMatrix = lookAt(camera.position, camera.target);
  const mat34 projectionMatrix =
      canvasProjectionMatrix(viewport.distance, canvas.width, canvas.height,
                             viewport.width, viewport.height);

  std::vector<Light> cameraLights;
  cameraLights.reserve(scene.lights.size());
  for (const Light &light : scene.lights) {
    cameraLights.push_back(transformLight(light, cameraMatrix));
  }

  const double near = 1.0;
  const std::vector<Plane> clippingPlanes = {
      // near
      {vec3{0.0, 0.0, 1.0}, -near},
      // left
      {normalize(vec3{viewport.distance, 0.0, viewport.width / 2.0}), 0.0},
      // right
      {normalize(vec3{-viewport.distance, 0.0, viewport.width / 2.0}), 0.0},
      // bottom
      {normalize(vec3{0.0, viewport.distance, viewport.height / 2.0}), 0.0},
      // top
      {normalize(vec3{0.0, -viewport.distance, viewport.height / 2.0}), 0.0},
  };

  const std::vector<ClippedInstance> clippedInstances =
      clipScene(scene, clippingPlanes, cameraMatrix);

  for (const ClippedInstance &instance : clippedInstances) {
    renderClippedInstance(canvas, instance, projectionMatrix, cameraLights);
  }
}
