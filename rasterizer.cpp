#include "rasterizer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace {
std::vector<double> interpolate(int i0, double d0, int i1, double d1) {
  // Interpolates d = f(i) between (i0, d0) and (i1, d1).
  if (i0 == i1) {
    return {d0};
  }

  std::vector<double> values;
  if (i0 < i1) {
    values.reserve(static_cast<std::size_t>(i1 - i0 + 1));
  }

  const double slope = (d1 - d0) / (i1 - i0);
  double value = d0;

  for (int i = i0; i <= i1; i++) {
    values.push_back(value);
    value += slope;
  }

  return values;
}

ScreenVertex snapToRaster(ScreenVertex vertex) {
  vertex.pos.x = rasterCoord(vertex.pos.x);
  vertex.pos.y = rasterCoord(vertex.pos.y);
  return vertex;
}

// One edge of the triangle, sampled per scanline.
// xs[i] is x at row y0 + i; attrs[a][i] is attribute a at the same row.
struct Edge {
  std::vector<double> xs;
  std::array<std::vector<double>, kAttributeCount> attrs;
};

Edge interpolateEdge(const ScreenVertex &p0, const ScreenVertex &p1) {
  Edge edge;
  const int y0 = rasterCoord(p0.pos.y);
  const int y1 = rasterCoord(p1.pos.y);

  edge.xs = interpolate(y0, p0.pos.x, y1, p1.pos.x);
  for (std::size_t attr = 0; attr < kAttributeCount; attr++) {
    edge.attrs[attr] = interpolate(y0, p0.attrs[attr], y1, p1.attrs[attr]);
  }

  return edge;
}

// Concatenate top->mid then mid->bottom, dropping the duplicated shared row.
Edge concatEdges(Edge top, const Edge &bottom) {
  top.xs.pop_back();
  top.xs.insert(top.xs.end(), bottom.xs.begin(), bottom.xs.end());

  for (std::size_t attr = 0; attr < kAttributeCount; attr++) {
    top.attrs[attr].pop_back();
    top.attrs[attr].insert(top.attrs[attr].end(), bottom.attrs[attr].begin(),
                           bottom.attrs[attr].end());
  }

  return top;
}
} // namespace

int rasterCoord(double value) { return static_cast<int>(std::lround(value)); }

void drawLine(Canvas &canvas, vec2 p0, vec2 p1, Color color) {
  int x0 = rasterCoord(p0.x);
  int y0 = rasterCoord(p0.y);
  int x1 = rasterCoord(p1.x);
  int y1 = rasterCoord(p1.y);

  const int dx = std::abs(x1 - x0);
  const int dy = std::abs(y1 - y0);

  if (dx > dy) {
    if (x0 > x1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    const std::vector<double> ys = interpolate(x0, y0, x1, y1);

    for (int x = x0; x <= x1; x++) {
      canvas.putPixel(x, rasterCoord(ys[x - x0]), toPixel(color));
    }
  } else {
    if (y0 > y1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    const std::vector<double> xs = interpolate(y0, x0, y1, x1);

    for (int y = y0; y <= y1; y++) {
      canvas.putPixel(rasterCoord(xs[y - y0]), y, toPixel(color));
    }
  }
}

void drawShadedTriangle(Canvas &canvas, ScreenVertex p0, ScreenVertex p1,
                        ScreenVertex p2, Color color) {
  p0 = snapToRaster(p0);
  p1 = snapToRaster(p1);
  p2 = snapToRaster(p2);

  if (p1.pos.y < p0.pos.y)
    std::swap(p1, p0);
  if (p2.pos.y < p0.pos.y)
    std::swap(p2, p0);
  if (p2.pos.y < p1.pos.y)
    std::swap(p2, p1);

  Edge shortEdge =
      concatEdges(interpolateEdge(p0, p1), interpolateEdge(p1, p2));
  Edge longEdge = interpolateEdge(p0, p2);

  const int midpoint = static_cast<int>(shortEdge.xs.size() / 2);
  const bool longEdgeIsLeft = longEdge.xs[midpoint] < shortEdge.xs[midpoint];
  const Edge &left = longEdgeIsLeft ? longEdge : shortEdge;
  const Edge &right = longEdgeIsLeft ? shortEdge : longEdge;

  const int y0 = rasterCoord(p0.pos.y);
  const int y2 = rasterCoord(p2.pos.y);
  for (int y = y0; y <= y2; y++) {
    const int row = y - y0;
    const int xLeft = rasterCoord(left.xs[row]);
    const int xRight = rasterCoord(right.xs[row]);

    std::array<std::vector<double>, kAttributeCount> segments;
    for (std::size_t attr = 0; attr < kAttributeCount; attr++) {
      segments[attr] = interpolate(xLeft, left.attrs[attr][row], xRight,
                                   right.attrs[attr][row]);
    }

    for (int x = xLeft; x <= xRight; x++) {
      const double illumination = segments[kIlluminationAttribute][x - xLeft];
      const double depth = segments[kDepthAttribute][x - xLeft];
      double currentDepth = 0.0;
      if (canvas.getDepth(x, y, currentDepth) && depth > currentDepth) {
        canvas.putPixel(x, y, toPixel(illumination * color));
        canvas.putDepth(x, y, depth);
      }
    }
  }
}

void drawWireframeTriangle(Canvas &canvas, vec2 p0, vec2 p1, vec2 p2,
                           Color color) {
  drawLine(canvas, p0, p1, color);
  drawLine(canvas, p0, p2, color);
  drawLine(canvas, p1, p2, color);
}
