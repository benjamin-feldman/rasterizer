#include <cassert>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

struct vec2 {
  int x, y;
};

struct Pixel {
  uint8_t r, g, b;
};

struct Canvas {
  int width, height;

  // Contiguous list of pixels
  // pixel (x, y) is at pixels[y*width + x]
  std::vector<Pixel> pixels;

  Canvas(int w, int h) : width(w), height(h), pixels(w * h) {
      for (Pixel& p : pixels){
          p = Pixel{255, 255, 255};
      }
  }

  void putPixelRaw(int x, int y, Pixel pixel) {
    // (x, y) in screen coordinates
    assert(x >= 0 && x < width);
    assert(y >= 0 && y < height);
    pixels[y * width + x] = pixel;
  }

  void putPixel(int x, int y, Pixel pixel) {
    // (x, y) in math coordinates
    int sx = width / 2 + x;
    int sy = height / 2 - y;
    putPixelRaw(sx, sy, pixel);
  }

  void save() {
    FILE *f = fopen("out.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    fwrite(pixels.data(), sizeof(Pixel), width * height, f);
    fclose(f);
  }
};

std::vector<float> interpolate(int i0, float d0, int i1, float d1) {

  if (i0 == i1) {
    return {d0};
  }

  std::vector<float> values;

  float a = (d1 - d0) / (i1 - i0);
  float d = d0;

  for (int i = i0; i <= i1; i++) {
    values.push_back(d);
    d = d + a;
  }

  return values;
}

void drawLine(Canvas &c, vec2 p0, vec2 p1, Pixel color) {
  int dx = abs(p1.x - p0.x);
  int dy = abs(p1.y - p0.y);

  if (dx > dy) {
    if (p0.x > p1.x) {
      std::swap(p0, p1);
    }

    auto ys = interpolate(p0.x, p0.y, p1.x, p1.y);

    for (int x = p0.x; x <= p1.x; x++) {
      c.putPixel(x, (int)ys[x - p0.x], color);
    }
  } else {
    if (p0.y > p1.y) {
      std::swap(p0, p1);
    }

    auto xs = interpolate(p0.y, p0.x, p1.y, p1.x);

    for (int y = p0.y; y <= p1.y; y++) {
      c.putPixel((int)xs[y - p0.y], y, color);
    }
  }
}

void drawFilledTriangle(Canvas &c, vec2 p0, vec2 p1, vec2 p2, Pixel color) {
  if (p1.y < p0.y)
    std::swap(p1, p0);
  if (p2.y < p0.y)
    std::swap(p2, p0);
  if (p2.y < p1.y)
    std::swap(p2, p1);

  std::vector<float> x01 = interpolate(p0.y, p0.x, p1.y, p1.x);
  std::vector<float> x12 = interpolate(p1.y, p1.x, p2.y, p2.x);
  std::vector<float> x02 = interpolate(p0.y, p0.x, p2.y, p2.x);

  x01.pop_back();
  std::vector<float> x012;
  x012.reserve(x01.size() + x12.size());
  x012.insert(x012.end(), x01.begin(), x01.end());
  x012.insert(x012.end(), x12.begin(), x12.end());

  int m = x012.size() / 2;

  std::vector<float> x_left;
  std::vector<float> x_right;

  if (x02[m] < x012[m]) {
    x_left = x02;
    x_right = x012;
  } else {
    x_left = x012;
    x_right = x02;
  }

  for (int y = p0.y; y <= p2.y; y++) {
    for (int x = (int)x_left[y - p0.y]; x <= (int)x_right[y - p0.y]; x++) {
      c.putPixel(x, y, color);
    }
  }
}

int main() {
  Canvas c(1000, 1000);

  Pixel color = {0, 255, 0};

  drawFilledTriangle(c, vec2{-200, -250}, vec2{200, 50}, vec2{20, 250}, color);

  c.save();
  return 0;
}
