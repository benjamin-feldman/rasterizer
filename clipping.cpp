#include "clipping.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <optional>
#include <utility>

namespace {
struct Sphere {
  vec3 center;
  double radius;
};

std::vector<ClipTriangle> trianglesToClipTriangles(
    const std::vector<vec4> &vertices, const std::vector<vec3> &vertexNormals,
    const std::vector<Triangle> &triangles,
    const std::vector<Material> &materials, const mat4 &transform) {
  std::vector<ClipTriangle> clipTriangles;
  clipTriangles.reserve(triangles.size());

  for (const Triangle &triangle : triangles) {
    assert(triangle.materialIndex >= 0 &&
           triangle.materialIndex < static_cast<int>(materials.size()));
    assert(triangle.vertexIndices[0] >= 0 &&
           triangle.vertexIndices[0] < static_cast<int>(vertices.size()));
    assert(triangle.vertexIndices[1] >= 0 &&
           triangle.vertexIndices[1] < static_cast<int>(vertices.size()));
    assert(triangle.vertexIndices[2] >= 0 &&
           triangle.vertexIndices[2] < static_cast<int>(vertices.size()));
    assert(triangle.vertexIndices[0] < static_cast<int>(vertexNormals.size()));
    assert(triangle.vertexIndices[1] < static_cast<int>(vertexNormals.size()));
    assert(triangle.vertexIndices[2] < static_cast<int>(vertexNormals.size()));

    clipTriangles.push_back(ClipTriangle{
        transform * vertices[triangle.vertexIndices[0]],
        transform * vertices[triangle.vertexIndices[1]],
        transform * vertices[triangle.vertexIndices[2]],
        transformNormal(transform, vertexNormals[triangle.vertexIndices[0]]),
        transformNormal(transform, vertexNormals[triangle.vertexIndices[1]]),
        transformNormal(transform, vertexNormals[triangle.vertexIndices[2]]),
        materials[triangle.materialIndex],
    });
  }

  return clipTriangles;
}

Sphere getBoundingSphere(const ClippedInstance &instance) {
  assert(!instance.triangles.empty());

  vec3 center{};
  int vertexCount = 0;
  for (const ClipTriangle &triangle : instance.triangles) {
    center = center + triangle.v0.xyz() + triangle.v1.xyz() + triangle.v2.xyz();
    vertexCount += 3;
  }
  center = center / vertexCount;

  double radiusSquared = 0.0;
  for (const ClipTriangle &triangle : instance.triangles) {
    const double d0 =
        dot(center - triangle.v0.xyz(), center - triangle.v0.xyz());
    const double d1 =
        dot(center - triangle.v1.xyz(), center - triangle.v1.xyz());
    const double d2 =
        dot(center - triangle.v2.xyz(), center - triangle.v2.xyz());

    if (d0 > radiusSquared)
      radiusSquared = d0;
    if (d1 > radiusSquared)
      radiusSquared = d1;
    if (d2 > radiusSquared)
      radiusSquared = d2;
  }

  return {center, std::sqrt(radiusSquared)};
}

double signedDistance(const Plane &plane, vec3 vertex) {
  return dot(plane.normal, vertex) + plane.d;
}

double intersectionParameter(const Plane &plane, vec3 a, vec3 b) {
  // Returns t such that lerp(a, b, t) lies on the plane. Clipping reuses this
  // value to interpolate both the new vertex position and its normal.
  return (-plane.d - dot(plane.normal, a)) / dot(plane.normal, b - a);
}

std::vector<ClipTriangle> clipTriangle(const ClipTriangle &triangle,
                                       const Plane &plane) {
  const std::array<vec4, 3> vertices{triangle.v0, triangle.v1, triangle.v2};
  const std::array<vec3, 3> normals{triangle.n0, triangle.n1, triangle.n2};
  const std::array<double, 3> distances{
      signedDistance(plane, vertices[0].xyz()),
      signedDistance(plane, vertices[1].xyz()),
      signedDistance(plane, vertices[2].xyz()),
  };

  const int positiveCount =
      (distances[0] >= 0.0) + (distances[1] >= 0.0) + (distances[2] >= 0.0);

  if (positiveCount == 3)
    return {triangle};
  if (positiveCount == 0)
    return {};

  if (positiveCount == 1) {
    const int a = (distances[0] >= 0.0) ? 0 : (distances[1] >= 0.0) ? 1 : 2;
    const int b = (a + 1) % 3;
    const int c = (a + 2) % 3;

    const double tb =
        intersectionParameter(plane, vertices[a].xyz(), vertices[b].xyz());
    const double tc =
        intersectionParameter(plane, vertices[a].xyz(), vertices[c].xyz());
    const vec4 bp = lerp(vertices[a], vertices[b], tb);
    const vec4 cp = lerp(vertices[a], vertices[c], tc);
    const vec3 bn = normalizeOrZero(lerp(normals[a], normals[b], tb));
    const vec3 cn = normalizeOrZero(lerp(normals[a], normals[c], tc));

    return {ClipTriangle{vertices[a], bp, cp, normals[a], bn, cn,
                         triangle.material}};
  }

  const int c = (distances[0] < 0.0) ? 0 : (distances[1] < 0.0) ? 1 : 2;
  const int a = (c + 1) % 3;
  const int b = (c + 2) % 3;

  const double ta =
      intersectionParameter(plane, vertices[a].xyz(), vertices[c].xyz());
  const double tb =
      intersectionParameter(plane, vertices[b].xyz(), vertices[c].xyz());
  const vec4 ap = lerp(vertices[a], vertices[c], ta);
  const vec4 bp = lerp(vertices[b], vertices[c], tb);
  const vec3 an = normalizeOrZero(lerp(normals[a], normals[c], ta));
  const vec3 bn = normalizeOrZero(lerp(normals[b], normals[c], tb));

  return {
      ClipTriangle{vertices[a], vertices[b], ap, normals[a], normals[b], an,
                   triangle.material},
      ClipTriangle{ap, vertices[b], bp, an, normals[b], bn, triangle.material},
  };
}

std::vector<ClipTriangle>
clipTrianglesAgainstPlane(const std::vector<ClipTriangle> &triangles,
                          const Plane &plane) {
  std::vector<ClipTriangle> clippedTriangles;
  clippedTriangles.reserve(triangles.size());

  for (const ClipTriangle &triangle : triangles) {
    for (const ClipTriangle &clippedTriangle : clipTriangle(triangle, plane)) {
      clippedTriangles.push_back(clippedTriangle);
    }
  }

  return clippedTriangles;
}

std::optional<ClippedInstance>
clipInstanceAgainstPlane(const ClippedInstance &instance,
                         const Sphere &boundingSphere, const Plane &plane) {
  const double distance = signedDistance(plane, boundingSphere.center);
  if (distance > boundingSphere.radius) {
    return instance;
  }
  if (distance < -boundingSphere.radius) {
    return std::nullopt;
  }

  return ClippedInstance{clipTrianglesAgainstPlane(instance.triangles, plane)};
}

std::optional<ClippedInstance> clipInstance(const ClippedInstance &instance,
                                            const std::vector<Plane> &planes) {
  const Sphere boundingSphere = getBoundingSphere(instance);
  ClippedInstance current = instance;

  for (const Plane &plane : planes) {
    std::optional<ClippedInstance> next =
        clipInstanceAgainstPlane(current, boundingSphere, plane);
    if (!next)
      return std::nullopt;
    current = std::move(*next);
  }

  return current;
}
}

std::vector<ClippedInstance> clipScene(const Scene &scene,
                                       const std::vector<Plane> &planes,
                                       const mat4 &cameraMatrix) {
  std::vector<ClippedInstance> clippedInstances;
  clippedInstances.reserve(scene.instances.size());

  for (const ModelInstance &instance : scene.instances) {
    assert(instance.model != nullptr);

    ClippedInstance initial{
        trianglesToClipTriangles(
            instance.model->vertices, instance.model->vertexNormals,
            instance.model->triangles, instance.model->materials,
            cameraMatrix * instance.transform),
    };

    if (std::optional<ClippedInstance> clipped =
            clipInstance(initial, planes)) {
      clippedInstances.push_back(std::move(*clipped));
    }
  }

  return clippedInstances;
}
