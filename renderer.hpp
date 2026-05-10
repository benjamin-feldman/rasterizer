#pragma once

#include "canvas.hpp"
#include "scene.hpp"

void renderScene(Canvas &canvas, const Viewport &viewport, const Camera &camera,
                 const Scene &scene);
