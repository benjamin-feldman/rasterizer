#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>

const bool DEBUG_AXES = true;
const double PI = 3.14159;

// Types

struct vec2 {
  double x, y;
};

vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
vec2 operator*(double s, vec2 a) { return {s * a.x, s * a.y}; }

struct vec4;

struct vec3 {
  union {
    struct {
      double x, y, z;
    };
    struct {
      double r, g, b;
    };
  };

  vec4 toVec4() const;
};

vec3 operator+(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 operator-(vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 operator*(double s, vec3 a) { return {s * a.x, s * a.y, s * a.z}; }
vec3 operator/(vec3 a, double s) { return {a.x / s, a.y / s, a.z / s}; }

double dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 cross(vec3 a, vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
vec3 normalize(vec3 a) { return (1.0 / sqrt(dot(a, a))) * a; }

using Color = vec3; // r,g,b in [0, 1]

struct Pixel {
  uint8_t r, g, b;
};

struct vec4 {
  double x, y, z, w;

  vec3 xyz() const { return vec3{x, y, z}; }
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

inline vec4 vec3::toVec4() const { return {x, y, z, 1}; }

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

struct Sphere {
  vec3 center;
  double r;
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

struct Plane {
  vec3 normal;
  double D;
};

struct ClipTri {
  // adhoc for the clipping pipeline
  vec4 v0;
  vec4 v1;
  vec4 v2;
  Color color;
};

std::vector<ClipTri> triToClipTri(const std::vector<vec4> &vertices,
                                  const std::vector<Triangle> &triangles,
                                  const mat4 &transform) {
  std::vector<ClipTri> clipTriangles;
  for (auto const &t : triangles) {
    ClipTri clipTri = {transform * vertices[t.idx[0]],
                       transform * vertices[t.idx[1]],
                       transform * vertices[t.idx[2]], t.color};
    clipTriangles.push_back(clipTri);
  }
  return clipTriangles;
}

struct ClippedInstance {
  std::vector<ClipTri> triangles;
};

Sphere getBoundingSphere(const ClippedInstance &instance) {
  vec3 center;
  int n = 0;
  for (const auto &t : instance.triangles) {
    center = center + t.v0.xyz() + t.v1.xyz() + t.v2.xyz();
    n += 3;
  }
  center = center / n;
  double r = 0;
  for (const auto &t : instance.triangles) {
    double d0 = dot(center - t.v0.xyz(), center - t.v0.xyz());
    double d1 = dot(center - t.v1.xyz(), center - t.v1.xyz());
    double d2 = dot(center - t.v2.xyz(), center - t.v2.xyz());
    if (d0 > r)
      r = d0;
    if (d1 > r)
      r = d1;
    if (d2 > r)
      r = d2;
  }
  return {center, sqrt(r)};
}

std::ostream &operator<<(std::ostream &os, vec2 v) {
  return os << "(" << v.x << ", " << v.y << ")";
}
std::ostream &operator<<(std::ostream &os, vec3 v) {
  return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}
std::ostream &operator<<(std::ostream &os, vec4 v) {
  return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}
std::ostream &operator<<(std::ostream &os, const ClipTri &t) {
  return os << "ClipTri{" << t.v0 << ", " << t.v1 << ", " << t.v2 << "}";
}
std::ostream &operator<<(std::ostream &os, const std::vector<ClipTri> &tris) {
  for (const auto &t : tris)
    os << t << "\n";
  return os;
}

double signedDistance(Plane plane, vec3 vertex) {
  return dot(plane.normal, vertex) + plane.D;
}

vec3 intersection(Plane plane, vec3 a, vec3 b) {
  // intersection between AB and plane (assuming existence of such intersection)
  double t = (-plane.D - dot(plane.normal, a)) / dot(plane.normal, b - a);
  return a + t * (b - a);
}

std::vector<ClipTri> clipTriangle(ClipTri triangle, Plane plane) {
  vec4 v[3] = {triangle.v0, triangle.v1, triangle.v2};
  double d[3] = {signedDistance(plane, v[0].xyz()),
                 signedDistance(plane, v[1].xyz()),
                 signedDistance(plane, v[2].xyz())};
  int nPos = (d[0] >= 0) + (d[1] >= 0) + (d[2] >= 0);

  if (nPos == 3)
    return {triangle};
  if (nPos == 0)
    return {};

  if (nPos == 1) {
    int a = (d[0] >= 0) ? 0 : (d[1] >= 0) ? 1 : 2;
    int b = (a + 1) % 3, c = (a + 2) % 3;
    vec3 bp = intersection(plane, v[a].xyz(), v[b].xyz());
    vec3 cp = intersection(plane, v[a].xyz(), v[c].xyz());
    return {ClipTri{v[a], bp.toVec4(), cp.toVec4(), triangle.color}};
  }

  int c = (d[0] < 0) ? 0 : (d[1] < 0) ? 1 : 2;
  int a = (c + 1) % 3, b = (c + 2) % 3;
  vec3 ap = intersection(plane, v[a].xyz(), v[c].xyz());
  vec3 bp = intersection(plane, v[b].xyz(), v[c].xyz());
  return {ClipTri{v[a], v[b], ap.toVec4(), triangle.color},
          ClipTri{ap.toVec4(), v[b], bp.toVec4(), triangle.color}};
}

std::vector<ClipTri>
clipTrianglesAgainstPlane(const std::vector<ClipTri> &triangles,
                          const Plane &plane) {
  std::vector<ClipTri> clippedTriangles;
  for (auto const &t : triangles) {
    for (auto const &clippedT : clipTriangle(t, plane)) {
      clippedTriangles.push_back(clippedT);
    }
  }
  return clippedTriangles;
}

ClippedInstance *clipInstanceAgainstPlane(const ClippedInstance &instance,
                                          const Sphere &boundingSphere,
                                          const Plane &plane) {
  double d = signedDistance(plane, boundingSphere.center);
  if (d > boundingSphere.r) {
    return new ClippedInstance(instance);
  } else if (d < -boundingSphere.r) {
    return nullptr;
  } else {
    return new ClippedInstance{
        clipTrianglesAgainstPlane(instance.triangles, plane)};
  }
}

ClippedInstance *clipInstance(const ClippedInstance &instance,
                              const std::vector<Plane> &planes) {
  Sphere boundingSphere = getBoundingSphere(instance);
  ClippedInstance *current = new ClippedInstance(instance);
  for (const auto &p : planes) {
    ClippedInstance *next =
        clipInstanceAgainstPlane(*current, boundingSphere, p);
    delete current;
    if (next == nullptr)
      return nullptr;
    current = next;
  }
  return current;
}

std::vector<ClippedInstance> clipScene(const std::vector<ModelInstance> &scene,
                                       const std::vector<Plane> &planes,
                                       const mat4 &cameraMatrix) {
  std::vector<ClippedInstance> clippedInstances;

  for (auto const &instance : scene) {
    ClippedInstance initial;
    initial.triangles =
        triToClipTri(instance.model.vertices, instance.model.triangles,
                     cameraMatrix * instance.transform);
    ClippedInstance *clipped = clipInstance(initial, planes);
    if (clipped != nullptr) {
      clippedInstances.push_back(*clipped);
      delete clipped;
    }
  }
  return clippedInstances;
}

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

void renderClippedInstance(Canvas &c, const ClippedInstance &instance,
                           const mat34 &projectionMatrix) {
  for (const auto &t : instance.triangles) {
    vec3 projected0 = projectionMatrix * t.v0;
    vec3 projected1 = projectionMatrix * t.v1;
    vec3 projected2 = projectionMatrix * t.v2;
    vec2 canvasVertex0 = {projected0.x / projected0.z,
                          projected0.y / projected0.z};
    vec2 canvasVertex1 = {projected1.x / projected1.z,
                          projected1.y / projected1.z};
    vec2 canvasVertex2 = {projected2.x / projected2.z,
                          projected2.y / projected2.z};
    drawWireframeTriangle(c, canvasVertex0, canvasVertex1, canvasVertex2,
                          t.color);
  }
}

struct Camera {
  vec3 position;
  vec3 target;
};

void renderScene(Canvas &c, Viewport &vp, Camera camera,
                 const std::vector<ModelInstance> &instances) {
  mat4 cameraMatrix = lookAt(camera.position, camera.target);
  mat34 projectionMatrix =
      canvasProjectionMatrix(vp.d, c.Cw, c.Ch, vp.Vw, vp.Vh);

  double Vw = vp.Vw, Vh = vp.Vh, d = vp.d;
  double near = 1.0;
  std::vector<Plane> clippingPlanes = {
      {vec3{0, 0, 1}, -near},              // near
      {normalize(vec3{d, 0, Vw / 2}), 0},  // left
      {normalize(vec3{-d, 0, Vw / 2}), 0}, // right
      {normalize(vec3{0, d, Vh / 2}), 0},  // bottom
      {normalize(vec3{0, -d, Vh / 2}), 0}, // top
  };

  std::vector<ClippedInstance> clippedInstances =
      clipScene(instances, clippingPlanes, cameraMatrix);

  for (const auto &instance : clippedInstances) {
    renderClippedInstance(c, instance, projectionMatrix);
  }
}

int main() {
  Canvas canvas(750, 750);
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

  mat4 transform_1 = translation(vec3{-2, 2, 10}) * rotationX(PI / 2) *
                     scaling(vec3{0.5, 0.5, 1});
  mat4 transform_2 = translation(vec3{2, 0, 8}) * rotationZ(PI / 3);
  ModelInstance cube_1(cube, transform_1);
  ModelInstance cube_2(cube, transform_2);

  std::vector<ModelInstance> scene = {cube_1, cube_2};

  Camera camera = {vec3{0, 0, -10}, vec3{2, 1, 1}};

  renderScene(canvas, vp, camera, scene);

  canvas.save();
  return 0;
}
