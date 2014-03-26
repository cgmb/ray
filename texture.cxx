#include <cmath>
#include "texture.h"

namespace algo_texture {
  vec3f checkerboard_3d(const vec3f& position,
    const vec3f& primary_color, const vec3f& secondary_color)
  {
    float x_value = std::floor(position[0]);
    float y_value = std::floor(position[1]);
    bool on = std::abs(std::fmod(x_value + y_value, 2.f)) < 1.f;
    float intensity = on ? 1.f : 0.f;
    return vec3f(intensity, intensity, intensity);
  }

  vec3f gridlines_3d(const vec3f& position, float period, float width,
    const vec3f& primary_color, const vec3f& secondary_color)
  {
    const float p = period;
    const float w = width;
    float z_value = std::floor(std::fmod(position[2], p) + p/2.f);

    float x_value = std::floor(std::fmod(position[0], p) + w);
    float y_value = std::floor(std::fmod(position[1] + z_value, p) + w);
    float intensity = x_value * y_value;
    return vec3f(intensity, intensity, intensity);
  }
}
