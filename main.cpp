#include <cassert>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

struct vec2 {
  int x, y;
};

struct Pixel {
  // TODO: make this inherit a vec3, with usual operators implemented
  uint8_t r, g, b;
};

struct ScreenVertex {
  vec2 pos;
  float h;
  ScreenVertex(vec2 pos, float h) : pos(pos), h(h) {}
};

struct Canvas {
  int width, height;

  // Contiguous list of pixels
  // pixel (x, y) is at pixels[y*width + x]
  std::vector<Pixel> pixels;

  Canvas(int w, int h)
      : width(w), height(h), pixels(w * h, Pixel{255, 255, 255}) {}

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

  // interpolate d = f(i) between (i0, d0) and (i1, d1)

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

void drawShadedTriangle(Canvas &c, ScreenVertex p0, ScreenVertex p1,
                        ScreenVertex p2, Pixel color) {
  if (p1.pos.y < p0.pos.y)
    std::swap(p1, p0);
  if (p2.pos.y < p0.pos.y)
    std::swap(p2, p0);
  if (p2.pos.y < p1.pos.y)
    std::swap(p2, p1);

  std::vector<float> x01 = interpolate(p0.pos.y, p0.pos.x, p1.pos.y, p1.pos.x);
  std::vector<float> h01 = interpolate(p0.pos.y, p0.h, p1.pos.y, p1.h);
  std::vector<float> x12 = interpolate(p1.pos.y, p1.pos.x, p2.pos.y, p2.pos.x);
  std::vector<float> h12 = interpolate(p1.pos.y, p1.h, p2.pos.y, p2.h);
  std::vector<float> x02 = interpolate(p0.pos.y, p0.pos.x, p2.pos.y, p2.pos.x);
  std::vector<float> h02 = interpolate(p0.pos.y, p0.h, p2.pos.y, p2.h);

  x01.pop_back();
  std::vector<float> x012;
  x012.reserve(x01.size() + x12.size());
  x012.insert(x012.end(), x01.begin(), x01.end());
  x012.insert(x012.end(), x12.begin(), x12.end());

  h01.pop_back();
  std::vector<float> h012;
  h012.reserve(h01.size() + h12.size());
  h012.insert(h012.end(), h01.begin(), h01.end());
  h012.insert(h012.end(), h12.begin(), h12.end());

  int m = x012.size() / 2;

  std::vector<float> x_left;
  std::vector<float> x_right;
  std::vector<float> h_left;
  std::vector<float> h_right;

  if (x02[m] < x012[m]) {
    x_left = x02;
    h_left = h02;

    x_right = x012;
    h_right = h012;
  } else {
    x_left = x012;
    h_left = h012;

    x_right = x02;
    h_right = h02;
  }

  for (int y = p0.pos.y; y <= p2.pos.y; y++) {
    int x_l = (int)x_left[y - p0.pos.y];
    int x_r = (int)x_right[y - p0.pos.y];

    std::vector<float> h_segment =
        interpolate(x_l, h_left[y - p0.pos.y], x_r, h_right[y - p0.pos.y]);

    for (int x = x_l; x <= x_r; x++) {
      float h = h_segment[x - x_l];
      Pixel shaded_color = {
          static_cast<uint8_t>(color.r * h),
          static_cast<uint8_t>(color.g * h),
          static_cast<uint8_t>(color.b * h),
      };

      c.putPixel(x, y, shaded_color);
    }
  }
}

int main() {
  Canvas c(1000, 1000);

  Pixel color = {0, 255, 0};
  ScreenVertex p0(vec2{-200, -250}, 0.0f);
  ScreenVertex p1(vec2{200, 50}, 1.0f);
  ScreenVertex p2(vec2{20, 250}, 0.5f);

  drawShadedTriangle(c, p0, p1, p2, color);

  c.save();
  return 0;
}
