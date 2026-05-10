#include "canvas.hpp"
#include "colors.hpp"
#include "math.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "scene.hpp"

namespace {
Scene makeDemoScene(const Model &head, const Model &cube) {
  const Light sun{
      LightType::Directional, {0.0, 10.0, -3.0}, {0, 1.0, 0.0}, 2.0};
  const Light ambient{LightType::Ambient, {}, {}, 0.7};

  const mat4 headTransform = translation(vec3{2.5, 1.5, -5.0}) *
                             rotationY(1.9 * PI / 2.0) *
                             scaling({10.0, 10.0, 10.0});
  const mat4 cube1Transform = translation(vec3{1.0, 2.0, -2.0}) *
                              rotationY(PI / 3.0) *
                              scaling(vec3{0.5, 0.5, 0.5});
  const mat4 cube2Transform = translation(vec3{0.0, -1.0, -6.0}) *
                              scaling({0.5, 0.5, 0.5}) * rotationY(PI / 6.0) *
                              rotationZ(PI / 6.0) * rotationX(7.0 * PI / 6.0);

  return Scene{
      {
          ModelInstance{&head, headTransform},
          ModelInstance{&cube, cube1Transform},
          ModelInstance{&cube, cube2Transform},
      },
      {sun, ambient},
  };
}
}

int main() {
  constexpr int canvasSize = 1000;

  const Viewport viewport{400.0, 400.0, 350.0};
  Canvas canvas(canvasSize, canvasSize, colors::white);

  const Model cube = makeCubeModel();
  const Model head = loadObj("head.OBJ", Material{colors::red, 1.0});

  // Scene parameters.
  const Camera camera{vec3{0.0, 0.0, -10.0}, vec3{2.0, 1.0, 1.0}};
  const Scene scene = makeDemoScene(head, cube);

  renderScene(canvas, viewport, camera, scene);
  canvas.save();
  return 0;
}
