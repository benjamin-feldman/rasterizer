#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

const bool DEBUG_AXES = true;

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

double dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 cross(vec3 a, vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
vec3 normalize(vec3 a) { return (1.0 / sqrt(dot(a, a))) * a; }

using Color = vec3; // r,g,b in [0, 1]
using Point3 = vec3;

struct Pixel {
  uint8_t r, g, b;
};

struct vec4 {
  double x, y, z, w;
};

struct mat4 {
  double m[4][4] = {};
};

mat4 operator*(const mat4 &a, const mat4 &b) {
  mat4 c;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        c.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }
  return c;
}

vec4 operator*(const mat4 &a, const vec4 &u) {
  double v[4];
  for (int i = 0; i < 4; i++) {
    v[i] =
        a.m[i][0] * u.x + a.m[i][1] * u.y + a.m[i][2] * u.z + a.m[i][3] * u.w;
  }
  return vec4{v[0], v[1], v[2], v[3]};
}

vec4 toVec4(vec3 v) { return {v.x, v.y, v.z, 1}; }

mat4 identity() {
  return {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

mat4 translation(vec3 t) {
  return {{{1, 0, 0, t.x}, {0, 1, 0, t.y}, {0, 0, 1, t.z}, {0, 0, 0, 1}}};
}

mat4 scaling(vec3 s) {
  return {{{s.x, 0, 0, 0}, {0, s.y, 0, 0}, {0, 0, s.z, 0}, {0, 0, 0, 1}}};
}

mat4 rotationX(double theta) {
  return {{{1, 0, 0, 0},
           {0, cos(theta), -sin(theta), 0},
           {0, sin(theta), cos(theta), 0},
           {0, 0, 0, 1}}};
}

mat4 rotationY(double theta) {
  return {{{cos(theta), 0, sin(theta), 0},
           {0, 1, 0, 0},
           {-sin(theta), 0, cos(theta), 0},
           {0, 0, 0, 1}}};
}

mat4 rotationZ(double theta) {
  return {{{cos(theta), -sin(theta), 0, 0},
           {sin(theta), cos(theta), 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1}}};
}

mat4 lookAt(vec3 eye, vec3 target, vec3 worldUp = {0, 1, 0}) {
  vec3 forward = normalize(target - eye);
  vec3 right = normalize(cross(worldUp, forward));
  vec3 up = cross(forward, right);
  return {{{right.x, right.y, right.z, -dot(right, eye)},
           {up.x, up.y, up.z, -dot(up, eye)},
           {forward.x, forward.y, forward.z, -dot(forward, eye)},
           {0, 0, 0, 1}}};
}

// mat34: only needed for perspective projection
struct mat34 {
  double m[3][4] = {};
};

vec3 operator*(const mat34 &a, const vec4 &u) {
  double v[3] = {};
  for (int i = 0; i < 3; i++) {
    v[i] =
        a.m[i][0] * u.x + a.m[i][1] * u.y + a.m[i][2] * u.z + a.m[i][3] * u.w;
  }
  return vec3{v[0], v[1], v[2]};
}

mat34 operator*(const mat34 &a, const mat4 &b) {
  mat34 c;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        c.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }
  return c;
}

mat34 canvasProjectionMatrix(double d, int Cw, int Ch, double Vw, double Vh) {
  // projects from Camera space to Canvas
  return {{{d * Cw / Vw, 0, 0, 0}, {0, d * Ch / Vh, 0, 0}, {0, 0, 1, 0}}};
}

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

struct Triangle {
  int idx[3];
  Color color;
};

struct Model {
  char name[16];
  std::vector<vec4> vertices;
  std::vector<Triangle> triangles;
};

struct ModelInstance {
  Model model;
  mat4 transform;

  ModelInstance(Model model, mat4 transform)
      : model(model), transform(transform) {}
};

struct Canvas {
  int Cw, Ch;

  // Contiguous list of pixels
  // pixel (x, y) is at pixels[y*width + x]
  std::vector<Pixel> pixels;

  Canvas(int w, int h) : Cw(w), Ch(h), pixels(w * h, Pixel{255, 255, 255}) {
    if (DEBUG_AXES)
      drawAxis();
  }

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

void drawWireframeTriangle(Canvas &c, vec2 p0, vec2 p1, vec2 p2, Color color) {
  drawLine(c, p0, p1, color);
  drawLine(c, p0, p2, color);
  drawLine(c, p1, p2, color);
}

void renderTriangle(Canvas &c, Triangle t,
                    const std::vector<vec2> &projectedVertices) {
  drawWireframeTriangle(c, projectedVertices[t.idx[0]],
                        projectedVertices[t.idx[1]],
                        projectedVertices[t.idx[2]], t.color);
}

void renderModel(Canvas &c, const ModelInstance &instance,
                 mat34 projectionMatrix) {
  std::vector<vec2> projectedVertices;
  projectedVertices.reserve(instance.model.vertices.size());
  mat34 transform = projectionMatrix * instance.transform;

  for (const auto &v : instance.model.vertices) {
    vec3 projectedVertex = transform * v;
    vec2 canvasVertex = {projectedVertex.x / projectedVertex.z,
                         projectedVertex.y / projectedVertex.z};
    projectedVertices.push_back(canvasVertex);
  }

  for (const auto &t : instance.model.triangles) {
    renderTriangle(c, t, projectedVertices);
  }
}

struct Camera {
  vec3 position;
  vec3 direction;
};

void renderScene(Canvas &c, Viewport &vp, Camera camera,
                 const std::vector<ModelInstance> &instances) {
  mat4 cameraMatrix = lookAt(camera.position, camera.direction);
  mat34 projectionMatrix =
      canvasProjectionMatrix(vp.d, c.Cw, c.Ch, vp.Vw, vp.Vh);
  mat34 fullProjection = projectionMatrix * cameraMatrix;
  for (const auto &instance : instances) {
    renderModel(c, instance, fullProjection);
  }
}

int main() {
  Canvas c(750, 750);
  Viewport vp(400, 400, 350);

  Color red = {1, 0, 0};
  Color green = {0, 1, 0};
  Color blue = {0, 0, 1};
  Color yellow = {1, 1, 0};
  Color cyan = {0.17, 1, 1};
  Color purple = {.5, .5, .5};

  std::vector<vec4> vertices = {{1, 1, 1, 1},    {-1, 1, 1, 1}, {-1, -1, 1, 1},
                                {1, -1, 1, 1},   {1, 1, -1, 1}, {-1, 1, -1, 1},
                                {-1, -1, -1, 1}, {1, -1, -1, 1}};

  std::vector<Triangle> triangles = {
      {{0, 1, 2}, red},    {{0, 2, 3}, red},    {{4, 0, 3}, green},
      {{4, 3, 7}, green},  {{5, 4, 7}, blue},   {{5, 7, 6}, blue},
      {{1, 5, 6}, yellow}, {{1, 6, 2}, yellow}, {{4, 5, 1}, purple},
      {{4, 1, 0}, purple}, {{2, 6, 7}, cyan},   {{2, 7, 3}, cyan}};

  Model cube = {"cube", vertices, triangles};

  double PI = 3.14159;
  mat4 transform_1 = translation(vec3{-2, 2, 10}) * rotationX(PI / 2) *
                     scaling(vec3{0.5, 0.5, 1});
  mat4 transform_2 = translation(vec3{2, 0, 8}) * rotationZ(PI / 3);
  ModelInstance cube_1(cube, transform_1);
  ModelInstance cube_2(cube, transform_2);

  std::vector<ModelInstance> scene = {cube_1, cube_2};

  Camera camera = {vec3{0, 0, -10}, vec3{2, 1, 1}};

  renderScene(c, vp, camera, scene);

  c.save();
  return 0;
}
