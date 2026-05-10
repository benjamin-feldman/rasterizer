#pragma once

#include "canvas.hpp"
#include "math.hpp"

#include <array>
#include <cstddef>

// Indices into ScreenVertex::attrs.
inline constexpr std::size_t kIlluminationAttribute = 0;
inline constexpr std::size_t kDepthAttribute = 1;
inline constexpr std::size_t kAttributeCount = 2;

struct ScreenVertex {
  vec2 pos;
  std::array<double, kAttributeCount> attrs{};
};

int rasterCoord(double value);
void drawLine(Canvas &canvas, vec2 p0, vec2 p1, Color color);
void drawShadedTriangle(Canvas &canvas, ScreenVertex p0, ScreenVertex p1,
                        ScreenVertex p2, Color color);
void drawWireframeTriangle(Canvas &canvas, vec2 p0, vec2 p1, vec2 p2,
                           Color color);
