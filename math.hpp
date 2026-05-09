#pragma once

#include <algorithm>
#include <cmath>

constexpr double PI = 3.14159;

struct vec2 {
  double x, y;
};

inline vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline vec2 operator*(double s, vec2 a) { return {s * a.x, s * a.y}; }

struct vec4;

struct vec3 {
  double x, y, z;

  vec4 toVec4() const;
};

inline vec3 operator+(vec3 a, vec3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline vec3 operator-(vec3 a, vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline vec3 operator*(double s, vec3 a) {
  return {s * a.x, s * a.y, s * a.z};
}
inline vec3 operator/(vec3 a, double s) {
  return {a.x / s, a.y / s, a.z / s};
}

inline double dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross(vec3 a, vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

inline vec3 normalize(vec3 a) { return (1.0 / std::sqrt(dot(a, a))) * a; }
inline double norm2(vec3 a) { return std::sqrt(dot(a, a)); }

inline vec3 normalizeOrZero(vec3 a) {
  double length = norm2(a);
  return length == 0 ? vec3{0, 0, 0} : a / length;
}

inline vec3 lerp(vec3 a, vec3 b, double t) {
  // t \in [0, 1]
  return a + t * (b - a);
}

struct vec4 {
  double x, y, z, w;

  vec3 xyz() const { return vec3{x, y, z}; }
};

inline vec4 lerp(vec4 a, vec4 b, double t) {
  return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
          a.z + t * (b.z - a.z), a.w + t * (b.w - a.w)};
}

struct mat4 {
  double m[4][4] = {};
};

inline mat4 operator*(const mat4 &a, const mat4 &b) {
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

inline vec4 operator*(const mat4 &a, const vec4 &u) {
  double v[4];
  for (int i = 0; i < 4; i++) {
    v[i] =
        a.m[i][0] * u.x + a.m[i][1] * u.y + a.m[i][2] * u.z + a.m[i][3] * u.w;
  }
  return vec4{v[0], v[1], v[2], v[3]};
}

inline vec3 transformNormal(const mat4 &transform, vec3 normal) {
  vec4 transformed = transform * vec4{normal.x, normal.y, normal.z, 0};
  return normalizeOrZero(transformed.xyz());
}

inline vec4 vec3::toVec4() const { return {x, y, z, 1}; }

inline mat4 translation(vec3 t) {
  return {{{1, 0, 0, t.x}, {0, 1, 0, t.y}, {0, 0, 1, t.z}, {0, 0, 0, 1}}};
}

inline mat4 scaling(vec3 s) {
  return {{{s.x, 0, 0, 0}, {0, s.y, 0, 0}, {0, 0, s.z, 0}, {0, 0, 0, 1}}};
}

inline mat4 rotationX(double theta) {
  return {{{1, 0, 0, 0},
           {0, std::cos(theta), -std::sin(theta), 0},
           {0, std::sin(theta), std::cos(theta), 0},
           {0, 0, 0, 1}}};
}

inline mat4 rotationY(double theta) {
  return {{{std::cos(theta), 0, std::sin(theta), 0},
           {0, 1, 0, 0},
           {-std::sin(theta), 0, std::cos(theta), 0},
           {0, 0, 0, 1}}};
}

inline mat4 rotationZ(double theta) {
  return {{{std::cos(theta), -std::sin(theta), 0, 0},
           {std::sin(theta), std::cos(theta), 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1}}};
}

inline mat4 lookAt(vec3 eye, vec3 target, vec3 worldUp = {0, 1, 0}) {
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

inline vec3 operator*(const mat34 &a, const vec4 &u) {
  double v[3] = {};
  for (int i = 0; i < 3; i++) {
    v[i] =
        a.m[i][0] * u.x + a.m[i][1] * u.y + a.m[i][2] * u.z + a.m[i][3] * u.w;
  }
  return vec3{v[0], v[1], v[2]};
}

inline mat34 operator*(const mat34 &a, const mat4 &b) {
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

inline mat34 canvasProjectionMatrix(double d, int Cw, int Ch, double Vw,
                                    double Vh) {
  // projects from Camera space to Canvas
  return {{{d * Cw / Vw, 0, 0, 0}, {0, d * Ch / Vh, 0, 0}, {0, 0, 1, 0}}};
}

inline double clamp(double x, double a, double b) {
  // returns x if x in [a, b], otherwise it returns the closest boundary
  return std::min(std::max(a, x), b);
}
