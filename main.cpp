#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "canvas.hpp"
#include "math.hpp"

constexpr Color RED = {1, 0, 0};
constexpr Color GREEN = {0, 1, 0};
constexpr Color BLUE = {0, 0, 1};
constexpr Color YELLOW = {1, 1, 0};
constexpr Color CYAN = {0.17, 1, 1};
constexpr Color PURPLE = {.5, .5, .5};
constexpr Color BLACK = {0, 0, 0};
constexpr Color WHITE = {1, 1, 1};

constexpr int MAT_RED = 0;
constexpr int MAT_GREEN = 1;
constexpr int MAT_BLUE = 2;
constexpr int MAT_YELLOW = 3;
constexpr int MAT_PURPLE = 4;
constexpr int MAT_CYAN = 5;

constexpr int FPS = 10;
constexpr int DURATION_SECONDS = 20;
constexpr int N_FRAMES = FPS * DURATION_SECONDS;

// Data types
enum LightType { POINT, DIRECTIONAL, AMBIENT };

struct Light {
  LightType sourceType;
  vec3 position;
  vec3 direction;
  double intensity;
};

struct Material {
  Color color;
  double specularity = -1;
};

struct Triangle {
  int verticesIdx[3];
  int materialIdx;
};

struct Model {
  char name[16];
  std::vector<vec4> vertices;
  std::vector<vec3> vertexNormals;
  std::vector<Triangle> triangles;
  std::vector<Material> materials;
};

struct ModelInstance {
  Model model;
  mat4 transform;

  ModelInstance(Model model, mat4 transform)
      : model(model), transform(transform) {}
};

struct Scene {
  std::vector<ModelInstance> instances;
  std::vector<Light> lights;
};

struct Camera {
  vec3 position;
  vec3 target;
};

struct Plane {
  vec3 normal;
  double D;
};

struct ClipTri {
  // Ad hoc triangle used in the clipping pipeline
  vec4 v0;
  vec4 v1;
  vec4 v2;
  vec3 n0;
  vec3 n1;
  vec3 n2;
  Material material;
};

struct ClippedInstance {
  std::vector<ClipTri> triangles;
};

struct Sphere {
  vec3 center;
  double r;
};

// Debug printing
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

// Lighting
double computeLighting(const vec3 &p, const vec3 &normal,
                       const vec3 &cameraDirection,
                       const std::vector<Light> &lights, double s) {
  double illumination = 0;

  for (const Light &light : lights) {
    if (light.sourceType == AMBIENT) {
      illumination += light.intensity;
      continue;
    }
    vec3 L = (light.sourceType == POINT) ? light.position - p : light.direction;

    // Diffuse
    double nDotL = dot(normal, L);
    if (nDotL > 0) {
      illumination += light.intensity * nDotL / (norm2(normal) * norm2(L));
    }

    // Specular
    if (s != -1 and nDotL > 0) {
      vec3 R = 2 * nDotL * normal - L;
      double rDotV = dot(R, cameraDirection);

      if (rDotV > 0) {
        illumination +=
            light.intensity *
            std::pow(rDotV / (norm2(R) * norm2(cameraDirection)), s);
      }
    }
  }
  return illumination;
}

// Model loading
void computeVertexNormals(Model &model) {
  model.vertexNormals.assign(model.vertices.size(), vec3{0, 0, 0});

  for (const Triangle &t : model.triangles) {
    assert(t.verticesIdx[0] >= 0 &&
           t.verticesIdx[0] < (int)model.vertices.size());
    assert(t.verticesIdx[1] >= 0 &&
           t.verticesIdx[1] < (int)model.vertices.size());
    assert(t.verticesIdx[2] >= 0 &&
           t.verticesIdx[2] < (int)model.vertices.size());

    vec3 v0 = model.vertices[t.verticesIdx[0]].xyz();
    vec3 v1 = model.vertices[t.verticesIdx[1]].xyz();
    vec3 v2 = model.vertices[t.verticesIdx[2]].xyz();
    vec3 faceNormal = normalizeOrZero(cross(v1 - v0, v2 - v0));
    model.vertexNormals[t.verticesIdx[0]] =
        model.vertexNormals[t.verticesIdx[0]] + faceNormal;
    model.vertexNormals[t.verticesIdx[1]] =
        model.vertexNormals[t.verticesIdx[1]] + faceNormal;
    model.vertexNormals[t.verticesIdx[2]] =
        model.vertexNormals[t.verticesIdx[2]] + faceNormal;
  }

  for (vec3 &normal : model.vertexNormals) {
    normal = normalizeOrZero(normal);
  }
}

Model loadOBJ(const char *path, Material material) {
  Model model = {};
  model.materials.push_back(material);
  const char *slash = std::strrchr(path, '/');
  const char *base = slash ? slash + 1 : path;
  std::snprintf(model.name, sizeof(model.name), "%s", base);

  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream ss(line);
    std::string token;
    ss >> token;

    if (token == "v") {
      double x, y, z;
      ss >> x >> y >> z;
      model.vertices.push_back({x, y, z, 1});
    } else if (token == "f") {
      std::vector<int> face;
      std::string vert;
      while (ss >> vert) {
        int idx = std::stoi(vert); // stoi stops at '/', so v/vt/vn just works
        if (idx < 0)
          idx = (int)model.vertices.size() + idx + 1;
        face.push_back(idx - 1); // OBJ is 1-based
      }
      for (int i = 1; i + 1 < (int)face.size(); i++)
        model.triangles.push_back({{face[0], face[i], face[i + 1]}, 0});
    }
  }
  computeVertexNormals(model);
  return model;
}

// Clipping
std::vector<ClipTri> triToClipTri(const std::vector<vec4> &vertices,
                                  const std::vector<vec3> &vertexNormals,
                                  const std::vector<Triangle> &triangles,
                                  const std::vector<Material> &materials,
                                  const mat4 &transform) {
  std::vector<ClipTri> clipTriangles;
  for (auto const &t : triangles) {
    assert(t.materialIdx >= 0 && t.materialIdx < (int)materials.size());
    assert(t.verticesIdx[0] >= 0 && t.verticesIdx[0] < (int)vertices.size());
    assert(t.verticesIdx[1] >= 0 && t.verticesIdx[1] < (int)vertices.size());
    assert(t.verticesIdx[2] >= 0 && t.verticesIdx[2] < (int)vertices.size());
    assert(t.verticesIdx[0] < (int)vertexNormals.size());
    assert(t.verticesIdx[1] < (int)vertexNormals.size());
    assert(t.verticesIdx[2] < (int)vertexNormals.size());

    ClipTri clipTri = {
        transform * vertices[t.verticesIdx[0]],
        transform * vertices[t.verticesIdx[1]],
        transform * vertices[t.verticesIdx[2]],
        transformNormal(transform, vertexNormals[t.verticesIdx[0]]),
        transformNormal(transform, vertexNormals[t.verticesIdx[1]]),
        transformNormal(transform, vertexNormals[t.verticesIdx[2]]),
        materials[t.materialIdx]};
    clipTriangles.push_back(clipTri);
  }
  return clipTriangles;
}

Sphere getBoundingSphere(const ClippedInstance &instance) {
  vec3 center = {};
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
  return {center, std::sqrt(r)};
}

// Plane clipping
double signedDistance(Plane plane, vec3 vertex) {
  return dot(plane.normal, vertex) + plane.D;
}

double intersectionParameter(Plane plane, vec3 a, vec3 b) {
  // returns t s.t. lerp(a, b, t) lies on the plane. Clipping needs this
  // value to interpolate both the new vertex position and its normal.
  return (-plane.D - dot(plane.normal, a)) / dot(plane.normal, b - a);
}

std::vector<ClipTri> clipTriangle(ClipTri triangle, Plane plane) {
  vec4 v[3] = {triangle.v0, triangle.v1, triangle.v2};
  vec3 n[3] = {triangle.n0, triangle.n1, triangle.n2};
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
    double tb = intersectionParameter(plane, v[a].xyz(), v[b].xyz());
    double tc = intersectionParameter(plane, v[a].xyz(), v[c].xyz());
    vec4 bp = lerp(v[a], v[b], tb);
    vec4 cp = lerp(v[a], v[c], tc);
    vec3 bn = normalizeOrZero(lerp(n[a], n[b], tb));
    vec3 cn = normalizeOrZero(lerp(n[a], n[c], tc));
    return {ClipTri{v[a], bp, cp, n[a], bn, cn, triangle.material}};
  }

  int c = (d[0] < 0) ? 0 : (d[1] < 0) ? 1 : 2;
  int a = (c + 1) % 3, b = (c + 2) % 3;
  double ta = intersectionParameter(plane, v[a].xyz(), v[c].xyz());
  double tb = intersectionParameter(plane, v[b].xyz(), v[c].xyz());
  vec4 ap = lerp(v[a], v[c], ta);
  vec4 bp = lerp(v[b], v[c], tb);
  vec3 an = normalizeOrZero(lerp(n[a], n[c], ta));
  vec3 bn = normalizeOrZero(lerp(n[b], n[c], tb));
  return {ClipTri{v[a], v[b], ap, n[a], n[b], an, triangle.material},
          ClipTri{ap, v[b], bp, an, n[b], bn, triangle.material}};
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

std::optional<ClippedInstance>
clipInstanceAgainstPlane(const ClippedInstance &instance,
                         const Sphere &boundingSphere, const Plane &plane) {
  double d = signedDistance(plane, boundingSphere.center);
  if (d > boundingSphere.r) {
    return instance;
  } else if (d < -boundingSphere.r) {
    return std::nullopt;
  } else {
    return ClippedInstance{
        clipTrianglesAgainstPlane(instance.triangles, plane)};
  }
}

std::optional<ClippedInstance> clipInstance(const ClippedInstance &instance,
                                            const std::vector<Plane> &planes) {
  Sphere boundingSphere = getBoundingSphere(instance);
  ClippedInstance current = instance;
  for (const auto &p : planes) {
    auto next = clipInstanceAgainstPlane(current, boundingSphere, p);
    if (!next)
      return std::nullopt;
    current = std::move(*next);
  }
  return current;
}

std::vector<ClippedInstance> clipScene(const Scene &scene,
                                       const std::vector<Plane> &planes,
                                       const mat4 &cameraMatrix) {
  std::vector<ClippedInstance> clippedInstances;

  for (auto const &instance : scene.instances) {
    ClippedInstance initial;
    initial.triangles =
        triToClipTri(instance.model.vertices, instance.model.vertexNormals,
                     instance.model.triangles, instance.model.materials,
                     cameraMatrix * instance.transform);
    if (auto clipped = clipInstance(initial, planes)) {
      clippedInstances.push_back(std::move(*clipped));
    }
  }
  return clippedInstances;
}

// Rasterization
// indices into ScreenVertex::attrs
constexpr int ATTR_H = 0;
constexpr int ATTR_DEPTH = 1;
constexpr int N_ATTRS = 2;

struct ScreenVertex {
  vec2 pos;
  std::vector<double> attrs; // size = N_ATTRS
  ScreenVertex(vec2 pos, std::vector<double> attrs)
      : pos(pos), attrs(std::move(attrs)) {}
};

int rasterCoord(double x) { return static_cast<int>(std::lround(x)); }

ScreenVertex snapToRaster(ScreenVertex p) {
  p.pos.x = rasterCoord(p.pos.x);
  p.pos.y = rasterCoord(p.pos.y);
  return p;
}

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
  int x0 = rasterCoord(p0.x);
  int y0 = rasterCoord(p0.y);
  int x1 = rasterCoord(p1.x);
  int y1 = rasterCoord(p1.y);

  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);

  if (dx > dy) {
    if (x0 > x1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    auto ys = interpolate(x0, y0, x1, y1);

    for (int x = x0; x <= x1; x++) {
      c.putPixel(x, rasterCoord(ys[x - x0]), toPixel(color));
    }
  } else {
    if (y0 > y1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    auto xs = interpolate(y0, x0, y1, x1);

    for (int y = y0; y <= y1; y++) {
      c.putPixel(rasterCoord(xs[y - y0]), y, toPixel(color));
    }
  }
}

// One edge of the triangle, sampled per-scanline
// xs[i] is x at row y0 + i; attrs[a][i] is attribute a at the same row
struct Edge {
  std::vector<double> xs;
  std::vector<std::vector<double>> attrs;
};

Edge interpEdge(const ScreenVertex &p0, const ScreenVertex &p1) {
  Edge e;
  int y0 = rasterCoord(p0.pos.y);
  int y1 = rasterCoord(p1.pos.y);
  e.xs = interpolate(y0, p0.pos.x, y1, p1.pos.x);
  e.attrs.reserve(N_ATTRS);
  for (int a = 0; a < N_ATTRS; a++)
    e.attrs.push_back(interpolate(y0, p0.attrs[a], y1, p1.attrs[a]));
  return e;
}

// Concatenate top->mid then mid->bot, dropping the duplicated shared row.
Edge concatEdges(Edge a, const Edge &b) {
  a.xs.pop_back();
  a.xs.insert(a.xs.end(), b.xs.begin(), b.xs.end());
  for (size_t i = 0; i < a.attrs.size(); i++) {
    a.attrs[i].pop_back();
    a.attrs[i].insert(a.attrs[i].end(), b.attrs[i].begin(), b.attrs[i].end());
  }
  return a;
}

void drawShadedTriangle(Canvas &c, ScreenVertex p0, ScreenVertex p1,
                        ScreenVertex p2, Color color) {
  p0 = snapToRaster(std::move(p0));
  p1 = snapToRaster(std::move(p1));
  p2 = snapToRaster(std::move(p2));

  if (p1.pos.y < p0.pos.y)
    std::swap(p1, p0);
  if (p2.pos.y < p0.pos.y)
    std::swap(p2, p0);
  if (p2.pos.y < p1.pos.y)
    std::swap(p2, p1);

  Edge shortEdge = concatEdges(interpEdge(p0, p1), interpEdge(p1, p2));
  Edge longEdge = interpEdge(p0, p2);

  int m = shortEdge.xs.size() / 2;
  bool longIsLeft = longEdge.xs[m] < shortEdge.xs[m];
  Edge &left = longIsLeft ? longEdge : shortEdge;
  Edge &right = longIsLeft ? shortEdge : longEdge;

  int y0 = rasterCoord(p0.pos.y);
  int y2 = rasterCoord(p2.pos.y);
  for (int y = y0; y <= y2; y++) {
    int row = y - y0;
    int x_l = rasterCoord(left.xs[row]);
    int x_r = rasterCoord(right.xs[row]);

    std::vector<std::vector<double>> segs;
    segs.reserve(N_ATTRS);
    for (int a = 0; a < N_ATTRS; a++)
      segs.push_back(
          interpolate(x_l, left.attrs[a][row], x_r, right.attrs[a][row]));

    for (int x = x_l; x <= x_r; x++) {
      double h = segs[ATTR_H][x - x_l];
      double d = segs[ATTR_DEPTH][x - x_l];
      double pixelCurrentDepth;
      if (c.getDepth(x, y, pixelCurrentDepth) && d > pixelCurrentDepth) {
        c.putPixel(x, y, toPixel(h * color));
        c.putDepth(x, y, d);
      }
    }
  }
}

void drawWireframeTriangle(Canvas &c, vec2 p0, vec2 p1, vec2 p2, Color color) {
  drawLine(c, p0, p1, color);
  drawLine(c, p0, p2, color);
  drawLine(c, p1, p2, color);
}

// Rendering
void renderClippedInstance(Canvas &c, const ClippedInstance &instance,
                           const mat34 &projectionMatrix,
                           const std::vector<Light> &lights) {
  for (const auto &t : instance.triangles) {
    // backface culling
    vec3 faceNormal = cross(t.v1.xyz() - t.v0.xyz(), t.v2.xyz() - t.v0.xyz());
    if (norm2(faceNormal) == 0)
      continue;
    faceNormal = normalize(faceNormal);
    if (dot(faceNormal, t.v0.xyz()) >= 0)
      continue;

    vec3 projected0 = projectionMatrix * t.v0;
    vec3 projected1 = projectionMatrix * t.v1;
    vec3 projected2 = projectionMatrix * t.v2;
    vec2 canvasVertex0 = {projected0.x / projected0.z,
                          projected0.y / projected0.z};
    vec2 canvasVertex1 = {projected1.x / projected1.z,
                          projected1.y / projected1.z};
    vec2 canvasVertex2 = {projected2.x / projected2.z,
                          projected2.y / projected2.z};
    // computeLighting expects: point position, surface normal, direction from
    // point to camera, scene lights, and material specularity. Vertices are in
    // camera space, so the camera is at the origin and point->camera is -point.
    double illumination0 = computeLighting(t.v0.xyz(), t.n0, -1 * t.v0.xyz(),
                                           lights, t.material.specularity);
    double illumination1 = computeLighting(t.v1.xyz(), t.n1, -1 * t.v1.xyz(),
                                           lights, t.material.specularity);
    double illumination2 = computeLighting(t.v2.xyz(), t.n2, -1 * t.v2.xyz(),
                                           lights, t.material.specularity);
    ScreenVertex screenVertex0(canvasVertex0,
                               {illumination0, 1.0 / projected0.z});
    ScreenVertex screenVertex1(canvasVertex1,
                               {illumination1, 1.0 / projected1.z});
    ScreenVertex screenVertex2(canvasVertex2,
                               {illumination2, 1.0 / projected2.z});
    drawShadedTriangle(c, screenVertex0, screenVertex1, screenVertex2,
                       t.material.color);
  }
}

Light transformLight(const Light &light, const mat4 &cameraMatrix) {
  Light out = light;
  if (light.sourceType == POINT) {
    vec4 p = cameraMatrix *
             vec4{light.position.x, light.position.y, light.position.z, 1};
    out.position = p.xyz();
  } else if (light.sourceType == DIRECTIONAL) {
    vec4 d = cameraMatrix *
             vec4{light.direction.x, light.direction.y, light.direction.z, 0};
    // here this is only a rotation thanks to w=0, because the cameraMatrix
    // passed is a lookAt matrix made of only translation+rotation, and we're
    // cancelling the rotation with w=0
    out.direction = d.xyz();
  }
  return out;
}

void renderScene(Canvas &c, const Viewport &vp, const Camera &camera,
                 const Scene &scene) {
  mat4 cameraMatrix = lookAt(camera.position, camera.target);
  mat34 projectionMatrix =
      canvasProjectionMatrix(vp.d, c.Cw, c.Ch, vp.Vw, vp.Vh);

  std::vector<Light> cameraLights;
  cameraLights.reserve(scene.lights.size());
  for (const Light &l : scene.lights)
    cameraLights.push_back(transformLight(l, cameraMatrix));

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
      clipScene(scene, clippingPlanes, cameraMatrix);

  for (const auto &instance : clippedInstances) {
    renderClippedInstance(c, instance, projectionMatrix, cameraLights);
  }
}

int main() {
  int canvasSize = 1000;

  Viewport vp(400, 400, 350);
  Canvas canvas(canvasSize, canvasSize, WHITE);

  std::vector<vec4> cubeVertices = {
      {1, 1, 1, 1},  {-1, 1, 1, 1},  {-1, -1, 1, 1},  {1, -1, 1, 1},
      {1, 1, -1, 1}, {-1, 1, -1, 1}, {-1, -1, -1, 1}, {1, -1, -1, 1}};

  // using default specularity for the cube faces
  std::vector<Material> cubeMaterials = {{RED},    {GREEN},  {BLUE},
                                         {YELLOW}, {PURPLE}, {CYAN}};

  std::vector<Triangle> cubeTriangles = {
      {{0, 1, 2}, MAT_RED},    {{0, 2, 3}, MAT_RED},    {{4, 0, 3}, MAT_GREEN},
      {{4, 3, 7}, MAT_GREEN},  {{5, 4, 7}, MAT_BLUE},   {{5, 7, 6}, MAT_BLUE},
      {{1, 5, 6}, MAT_YELLOW}, {{1, 6, 2}, MAT_YELLOW}, {{4, 5, 1}, MAT_PURPLE},
      {{4, 1, 0}, MAT_PURPLE}, {{2, 6, 7}, MAT_CYAN},   {{2, 7, 3}, MAT_CYAN}};

  Model cube = {"cube", cubeVertices, {}, cubeTriangles, cubeMaterials};
  computeVertexNormals(cube);

  Model head = loadOBJ("head.OBJ", Material{RED, 1});

  // Scene params
  Camera camera = {vec3{0, 0, -10}, vec3{2, 1, 1}};
  Light sun = {DIRECTIONAL, {0, 10, -3}, {0.3, 1, 0}, 2};
  Light ambient = {AMBIENT, {}, {}, 0.7};

  mat4 headTransform = translation(vec3{2.5, 1.5, -5}) *
                       rotationY(1.9 * PI / 2) * scaling({10, 10, 10});

  mat4 cube1Transform = translation(vec3{1, 2, -2}) * rotationY(PI / 3) *
                        scaling(vec3{0.5, 0.5, 0.5});
  mat4 cube2Transform = translation(vec3{0, -1, -6}) *
                        scaling({0.5, 0.5, 0.5}) * rotationY(PI / 6) *
                        rotationZ(PI / 6) * rotationX(7 * PI / 6);

  Scene scene = {{ModelInstance(head, headTransform),
                  ModelInstance(cube, cube1Transform),
                  ModelInstance(cube, cube2Transform)},
                 {sun, ambient}};

  renderScene(canvas, vp, camera, scene);

  canvas.save();
  return 0;
}
