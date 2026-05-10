#include "lighting.hpp"

#include <cmath>

double computeLighting(const vec3 &point, const vec3 &normal,
                       const vec3 &cameraDirection,
                       const std::vector<Light> &lights, double specularity) {
  double illumination = 0.0;

  for (const Light &light : lights) {
    if (light.type == LightType::Ambient) {
      illumination += light.intensity;
      continue;
    }

    const vec3 lightVector = (light.type == LightType::Point)
                                 ? light.position - point
                                 : light.direction;

    // Diffuse reflection.
    const double nDotL = dot(normal, lightVector);
    if (nDotL > 0.0) {
      illumination +=
          light.intensity * nDotL / (norm2(normal) * norm2(lightVector));
    }

    // Specular reflection.
    if (specularity != -1.0 && nDotL > 0.0) {
      const vec3 reflection = 2.0 * nDotL * normal - lightVector;
      const double rDotV = dot(reflection, cameraDirection);

      if (rDotV > 0.0) {
        illumination +=
            light.intensity *
            std::pow(rDotV / (norm2(reflection) * norm2(cameraDirection)),
                     specularity);
      }
    }
  }

  return illumination;
}

Light transformLight(const Light &light, const mat4 &cameraMatrix) {
  Light transformed = light;
  if (light.type == LightType::Point) {
    const vec4 position =
        cameraMatrix *
        vec4{light.position.x, light.position.y, light.position.z, 1.0};
    transformed.position = position.xyz();
  } else if (light.type == LightType::Directional) {
    const vec4 direction =
        cameraMatrix *
        vec4{light.direction.x, light.direction.y, light.direction.z, 0.0};
    // w=0 applies only the camera rotation, not its translation.
    transformed.direction = direction.xyz();
  }
  return transformed;
}
