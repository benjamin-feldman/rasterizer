#include <cassert>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

// Types

struct vec2 {
  double x, y;
};

vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
vec2 operator*(double s, vec2 a) { return {s * a.x, s * a.y}; }

struct vec3 {
  union {
    struct {
      double x, y, z;
    };
    struct {
      double r, g, b;
    };
  };
};

vec3 operator+(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 operator-(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 operator*(double s, vec3 a) { return {s * a.x, s * a.y, s * a.z}; }

using Color = vec3; // r,g,b in [0, 1]
using Point3 = vec3;

struct Pixel {
  uint8_t r, g, b;
};

double clamp(double x, double a, double b) {
  // returns x if x in [a, b], otherwise it returns the closest boundary
  return std::min(std::max(a, x), b);
}

Pixel toPixel(Color c) {
  return {
      static_cast<uint8_t>(clamp(c.r, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.g, 0.0, 1.0) * 255),
      static_cast<uint8_t>(clamp(c.b, 0.0, 1.0) * 255),
  };
}

struct ScreenVertex {
  vec2 pos;
  double h;
  ScreenVertex(vec2 pos, double h) : pos(pos), h(h) {}
};

struct Canvas {
  int Cw, Ch;

  // Contiguous list of pixels
  // pixel (x, y) is at pixels[y*width + x]
  std::vector<Pixel> pixels;

  Canvas(int w, int h) : Cw(w), Ch(h), pixels(w * h, Pixel{255, 255, 255}) {}

  void putPixelRaw(int x, int y, Pixel pixel) {
    // (x, y) in screen coordinates
    assert(x >= 0 && x < Cw);
    assert(y >= 0 && y < Ch);
    pixels[y * Cw + x] = pixel;
  }

  void putPixel(int x, int y, Pixel pixel) {
    // (x, y) in math coordinates
    int sx = Cw / 2 + x;
    int sy = Ch / 2 - y;
    putPixelRaw(sx, sy, pixel);
  }

  void save() {
    FILE *f = fopen("out.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", Cw, Ch);
    fwrite(pixels.data(), sizeof(Pixel), Cw * Ch, f);
    fclose(f);
  }
};

struct Viewport {
  double Vw, Vh, d;
  Viewport(int w, int h, double d) : Vw(w), Vh(h), d(d) {};

  vec2 toCanvas(vec2 p, const Canvas &c) const {
    return {p.x * c.Cw / Vw, p.y * c.Ch / Vh};
  }

  vec2 projectVertex(vec3 v, const Canvas &c) const {
    return toCanvas({v.x * d / v.z, v.y * d / v.z}, c);
  }
};

std::vector<double> interpolate(int i0, double d0, int i1, double d1) {

  // interpolate d = f(i) between (i0, d0) and (i1, d1)

  if (i0 == i1) {
    return {d0};
  }

  std::vector<double> values;

  double a = (d1 - d0) / (i1 - i0);
  double d = d0;

  for (int i = i0; i <= i1; i++) {
    values.push_back(d);
    d = d + a;
  }

  return values;
}

void drawLine(Canvas &c, vec2 p0, vec2 p1, Color color) {
  double dx = abs(p1.x - p0.x);
  double dy = abs(p1.y - p0.y);

  if (dx > dy) {
    if (p0.x > p1.x) {
      std::swap(p0, p1);
    }

    auto ys = interpolate(p0.x, p0.y, p1.x, p1.y);

    for (int x = p0.x; x <= p1.x; x++) {
      c.putPixel(x, (int)ys[x - p0.x], toPixel(color));
    }
  } else {
    if (p0.y > p1.y) {
      std::swap(p0, p1);
    }

    auto xs = interpolate(p0.y, p0.x, p1.y, p1.x);

    for (int y = p0.y; y <= p1.y; y++) {
      c.putPixel((int)xs[y - p0.y], y, toPixel(color));
    }
  }
}

void drawShadedTriangle(Canvas &c, ScreenVertex p0, ScreenVertex p1,
                        ScreenVertex p2, Color color) {
  if (p1.pos.y < p0.pos.y)
    std::swap(p1, p0);
  if (p2.pos.y < p0.pos.y)
    std::swap(p2, p0);
  if (p2.pos.y < p1.pos.y)
    std::swap(p2, p1);

  std::vector<double> x01 = interpolate(p0.pos.y, p0.pos.x, p1.pos.y, p1.pos.x);
  std::vector<double> h01 = interpolate(p0.pos.y, p0.h, p1.pos.y, p1.h);
  std::vector<double> x12 = interpolate(p1.pos.y, p1.pos.x, p2.pos.y, p2.pos.x);
  std::vector<double> h12 = interpolate(p1.pos.y, p1.h, p2.pos.y, p2.h);
  std::vector<double> x02 = interpolate(p0.pos.y, p0.pos.x, p2.pos.y, p2.pos.x);
  std::vector<double> h02 = interpolate(p0.pos.y, p0.h, p2.pos.y, p2.h);

  x01.pop_back();
  std::vector<double> x012;
  x012.reserve(x01.size() + x12.size());
  x012.insert(x012.end(), x01.begin(), x01.end());
  x012.insert(x012.end(), x12.begin(), x12.end());

  h01.pop_back();
  std::vector<double> h012;
  h012.reserve(h01.size() + h12.size());
  h012.insert(h012.end(), h01.begin(), h01.end());
  h012.insert(h012.end(), h12.begin(), h12.end());

  int m = x012.size() / 2;

  std::vector<double> x_left;
  std::vector<double> x_right;
  std::vector<double> h_left;
  std::vector<double> h_right;

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

    std::vector<double> h_segment =
        interpolate(x_l, h_left[y - p0.pos.y], x_r, h_right[y - p0.pos.y]);

    for (int x = x_l; x <= x_r; x++) {
      float h = h_segment[x - x_l];
      Pixel shaded_pixel = toPixel(h * color);
      c.putPixel(x, y, shaded_pixel);
    }
  }
}

int main() {
  Canvas c(1000, 1000);
  Viewport vp(400, 400, 50);

  Color color = {0, 1, 0};
  ScreenVertex p0(vec2{-200, -250}, 0.0f);
  ScreenVertex p1(vec2{200, 50}, 1.0f);
  ScreenVertex p2(vec2{20, 250}, 0.5f);

  drawShadedTriangle(c, p0, p1, p2, color);

  // The four "front" vertices
  vec3 vAf = {-1, 2, 1};
  vec3 vBf = {1, 2, 1};
  vec3 vCf = {1, -1, 1};
  vec3 vDf = {-1, -1, 1};
  // The four "back" vertices
  vec3 vAb = {-3, 1, 2};
  vec3 vBb = {-1, 1, 2};
  vec3 vCb = {-1, -1, 2};
  vec3 vDb = {-3, -1, 2};

  Color red = {1, 0, 0};
  Color green = {0, 1, 0};
  Color blue = {0, 0, 1};

  drawLine(c, vp.projectVertex(vAf, c), vp.projectVertex(vBf, c), blue);
  drawLine(c, vp.projectVertex(vBf, c), vp.projectVertex(vCf, c), blue);
  drawLine(c, vp.projectVertex(vCf, c), vp.projectVertex(vDf, c), blue);
  drawLine(c, vp.projectVertex(vDf, c), vp.projectVertex(vAf, c), blue);

  drawLine(c, vp.projectVertex(vAb, c), vp.projectVertex(vBb, c), red);
  drawLine(c, vp.projectVertex(vBb, c), vp.projectVertex(vCb, c), red);
  drawLine(c, vp.projectVertex(vCb, c), vp.projectVertex(vDb, c), red);
  drawLine(c, vp.projectVertex(vDb, c), vp.projectVertex(vAb, c), red);

  drawLine(c, vp.projectVertex(vAf, c), vp.projectVertex(vAb, c), green);
  drawLine(c, vp.projectVertex(vBf, c), vp.projectVertex(vBb, c), green);
  drawLine(c, vp.projectVertex(vCf, c), vp.projectVertex(vCb, c), green);
  drawLine(c, vp.projectVertex(vDf, c), vp.projectVertex(vDb, c), green);

  c.save();
  return 0;
}
