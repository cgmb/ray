#include <algorithm>
#include <functional>
#include <limits>
#include <vector>
#include "geometry.h"
#include "image.h"
#include "vec3f.h"

using std::placeholders::_1;

struct resolution {
  unsigned x;
  unsigned y;
};

struct light {
  vec3f position;
  vec3f color;
};

struct ray_sphere_intersect {
  float t;
  typename std::vector<sphere>::const_iterator near_geometry_it;

  bool intersect_exists(const std::vector<sphere>& s) const {
    return !std::isnan(t) && s.end() != near_geometry_it;
  }
};

float quiet_nan() {
  static_assert(std::numeric_limits<float>::has_quiet_NaN,
    "Implementation requires NaN");
  return std::numeric_limits<float>::quiet_NaN();
}

ray_sphere_intersect cast_ray(
  const ray& eye_ray,
  const std::vector<sphere>& geometry)
{
  ray_sphere_intersect rsi = { quiet_nan(), geometry.end() };
  if (geometry.empty()) {
    return rsi;
  }

  auto intersects_eye_ray_at = std::bind(near_intersect_param, eye_ray, _1);

  std::vector<float> intersections(geometry.size());
  std::transform(geometry.begin(), geometry.end(), 
    intersections.begin(), intersects_eye_ray_at);
  auto near_it = std::min_element(intersections.cbegin(), intersections.cend());
  auto near_geometry_it = geometry.begin() +
    std::distance(intersections.cbegin(), near_it);
  rsi.t = *near_it;
  rsi.near_geometry_it = near_geometry_it;
  return rsi;
}

int main(int argc, char** argv) {
  vec3f observer = { 0, 0, -10 };
  vec3f screen_top_left = { -5, 5, 0 };
  vec3f screen_top_right = { 5, 5, 0 };
  vec3f screen_bottom_right = { 5, -5, 0 };
  resolution res = { 100, 100 };

  vec3f screen_offset_x = screen_top_right - screen_top_left;
  vec3f screen_offset_y = screen_bottom_right - screen_top_right;

  vec3f screen_offset_per_px_x = (1.f / res.x) * screen_offset_x;
  vec3f screen_offset_per_px_y = (1.f / res.y) * screen_offset_y;

  image img(res.x, res.y);

  std::vector<sphere> geometry;
  sphere s = { vec3f(0, 0, 10), 3 };
  geometry.push_back(s);

  std::vector<light> lights;
  light l = { vec3f(0, 0, -10), vec3f(1, 1, 1) };
  lights.push_back(l);

  for (unsigned y = 0u; y < res.y; ++y) {
    for (unsigned x = 0u; x < res.x; ++x) {
      vec3f color = { 0, 0, 0 };
      vec3f pixel_pos = screen_top_left +
        x * screen_offset_per_px_x +
        y * screen_offset_per_px_y;
      ray eye_ray = { observer, normalized(pixel_pos - observer) };
      ray_sphere_intersect rsi = cast_ray(eye_ray, geometry);
      if (rsi.intersect_exists(geometry)) {
//        vec3f pos = eye_ray.position_at(rsi.t);
        color = vec3f(1, 0, 0);
      }

      img.px(x, y) = color;
    }
  }

  if (!img.save_as_png("output.png")) {
    return 1;
  }
  
  return 0;
}
