#pragma once

#include "math.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr bool DEBUG_AXES = false;

using Color = vec3; // r,g,b in [0, 1]

struct Pixel {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
static_assert(sizeof(Pixel) == 3);

inline Pixel toPixel(Color c) {
  return {
      static_cast<uint8_t>(clamp(c.x, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.y, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.z, 0.0, 1.0) * 255),
  };
}

struct Canvas {
  int width = 0;
  int height = 0;
  // Contiguous row-major storage: pixel (x, y) is pixels[y * width + x].
  std::vector<Pixel> pixels;
  // Same layout as pixels
  std::vector<double> depths;
  Color backgroundColor;

  explicit Canvas(int canvasWidth, int canvasHeight,
                  Color backgroundColor = {1, 1, 1})
      : width(canvasWidth), height(canvasHeight),
        pixels(pixelCountFor(canvasWidth, canvasHeight),
               toPixel(backgroundColor)),
        depths(pixelCountFor(canvasWidth, canvasHeight), 0.0),
        backgroundColor(backgroundColor) {
    if (DEBUG_AXES)
      drawAxis();
  }

  static std::size_t pixelCountFor(int width, int height) {
    assert(width > 0);
    assert(height > 0);
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  }

  std::size_t pixelIndex(int sx, int sy) const {
    assert(isInBounds(sx, sy));
    return static_cast<std::size_t>(sy) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(sx);
  }

  bool isInBounds(int sx, int sy) const {
    return sx >= 0 && sx < width && sy >= 0 && sy < height;
  }

  bool toScreen(int x, int y, int &sx, int &sy) const {
    sx = width / 2 + x;
    sy = height / 2 - y;
    return isInBounds(sx, sy);
  }

  void putPixelRaw(int sx, int sy, Pixel pixel) {
    pixels[pixelIndex(sx, sy)] = pixel;
  }

  void putPixel(int x, int y, Pixel pixel) {
    // (x, y) in math coordinates
    int sx, sy;
    if (!toScreen(x, y, sx, sy)) {
      return;
    }
    putPixelRaw(sx, sy, pixel);
  }

  void putDepthRaw(int sx, int sy, double depth) {
    depths[pixelIndex(sx, sy)] = depth;
  }

  void putDepth(int x, int y, double depth) {
    int sx, sy;
    if (!toScreen(x, y, sx, sy)) {
      return;
    }
    putDepthRaw(sx, sy, depth);
  }

  bool getDepth(int x, int y, double &depth) const {
    int sx, sy;
    if (!toScreen(x, y, sx, sy)) {
      return false;
    }
    depth = depths[pixelIndex(sx, sy)];
    return true;
  }

  void drawAxis() {
    int midW = width / 2;
    int midH = height / 2;
    Pixel grey = {150, 150, 150};
    for (int i = 0; i < width; i++) {
      putPixelRaw(i, midH, grey);
    }

    for (int j = 0; j < height; j++) {
      putPixelRaw(midW, j, grey);
    }
  }

  void save(const std::string &path = "out.ppm") const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
      throw std::runtime_error("could not open output file: " + path);
    }

    file << "P6\n" << width << ' ' << height << "\n255\n";
    file.write(reinterpret_cast<const char *>(pixels.data()),
               static_cast<std::streamsize>(pixels.size() * sizeof(Pixel)));
    if (!file) {
      throw std::runtime_error("could not write output file: " + path);
    }
  }
};

struct Viewport {
  double width;
  double height;
  double distance;
};
