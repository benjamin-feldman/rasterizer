#pragma once

#include "math.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr bool DEBUG_AXES = false;

using Color = vec3; // r,g,b in [0, 1]

struct Pixel {
  uint8_t r, g, b;
};

inline Pixel toPixel(Color c) {
  return {
      static_cast<uint8_t>(clamp(c.x, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.y, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.z, 0.0, 1.0) * 255),
  };
}

struct Canvas {
  int Cw, Ch;
  // Contiguous list of pixels
  // pixel (x, y) is at pixels[y*width + x]
  std::vector<Pixel> pixels;
  // same for depths
  std::vector<double> depths;
  Color backgroundColor;

  Canvas(int w, int h, Color backgroundColor = {1, 1, 1})
      : Cw(w), Ch(h), pixels(w * h, toPixel(backgroundColor)),
        depths(w * h, 0) {
    if (DEBUG_AXES)
      drawAxis();
  }

  bool isInBounds(int sx, int sy) const {
    return sx >= 0 && sx < Cw && sy >= 0 && sy < Ch;
  }

  bool toScreen(int x, int y, int &sx, int &sy) const {
    sx = Cw / 2 + x;
    sy = Ch / 2 - y;
    return isInBounds(sx, sy);
  }

  void putPixelRaw(int sx, int sy, Pixel pixel) {
    assert(isInBounds(sx, sy));
    pixels[sy * Cw + sx] = pixel;
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
    assert(isInBounds(sx, sy));
    depths[sy * Cw + sx] = depth;
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
    depth = depths[sy * Cw + sx];
    return true;
  }

  void drawAxis() {
    int midW = Cw / 2;
    int midH = Ch / 2;
    Pixel grey = {150, 150, 150};
    for (int i = 0; i < Cw; i++) {
      putPixelRaw(i, midH, grey);
    }

    for (int j = 0; j < Ch; j++) {
      putPixelRaw(midW, j, grey);
    }
  }

  void save(const char *path = "out.ppm") {
    std::FILE *f = std::fopen(path, "wb");
    std::fprintf(f, "P6\n%d %d\n255\n", Cw, Ch);
    std::fwrite(pixels.data(), sizeof(Pixel), Cw * Ch, f);
    std::fclose(f);
  }
};

struct Viewport {
  double Vw, Vh, d;
  Viewport(int w, int h, double d) : Vw(w), Vh(h), d(d) {};
};
