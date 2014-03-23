#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <algorithm>
#include <functional>
#include <limits>
#include <cassert>
#include <cmath>
#include <vector>
#include "vec3f.h"

using std::placeholders::_1;

struct ray_t {
  static ray_t from_point_vector(const vec3f& start, const vec3f& direction) {
    return ray_t{ start, direction };
  }

  // returns the position of the ray at t multiples of the ray direction.
  vec3f position_at(float t) const {
    return start + (direction * t);
  }

  vec3f start;
  vec3f direction;
};

struct sphere_t {
  static sphere_t from_center_radius_squared(
    const vec3f& center, float radius_squared)
  {
    return sphere_t{ center, radius_squared };
  }

  vec3f normal_at(const vec3f& position) const {
    return normalized(position - center);
  }

  vec3f center;
  float radius_squared;
};

// todo: move to more appropriate header
inline bool abs_fuzzy_eq(double lhs, double rhs, double abs_epsilon) {
  return std::abs(lhs - rhs) < abs_epsilon;
}

inline float quiet_nan() {
  static_assert(std::numeric_limits<float>::has_quiet_NaN,
    "Implementation requires NaN");
  return std::numeric_limits<float>::quiet_NaN();
}

// returns the t value for the near intersect point
// along the parametric equation of the ray (pos = origin + direction * t)
inline float near_intersect_param(const ray_t& r, const sphere_t& s) {
  assert(abs_fuzzy_eq(magnitude(r.direction), 1, 1e-3));

  const vec3f m = r.start - s.center;
  const vec3f d = r.direction;

  float md = dot(m,d);
  float dd = 1.f; // dot(d,d);
  float c = std::sqrt(md*md - (dd * (dot(m,m) - s.radius_squared)));
  if (std::isnan(c)) {
    return c;
  }

  float x1 = (-md - c) / dd;
  float x2 = (-md + c) / dd;
  if (x2 < 0.f) {
    return quiet_nan();
  } else if (x1 < 0.f) {
    return x2;
  } else {
    return x1;
  }
}

inline vec3f near_intersect(const ray_t& r, const sphere_t& s) {
  return r.start + (near_intersect_param(r, s) * r.direction);
}

struct ray_sphere_intersect {
  float t;
  typename std::vector<sphere_t>::const_iterator near_geometry_it;

  bool intersect_exists(const std::vector<sphere_t>& s) const {
    return !std::isnan(t) && s.end() != near_geometry_it;
  }

  size_t index_in(const std::vector<sphere_t>& s) const {
    return std::distance(s.begin(), near_geometry_it);
  }
};

inline ray_sphere_intersect cast_ray(
  const ray_t& eye_ray,
  const std::vector<sphere_t>& geometry)
{
  ray_sphere_intersect rsi = { quiet_nan(), geometry.end() };
  if (geometry.empty()) {
    return rsi;
  }

  auto intersects_eye_ray_at = std::bind(near_intersect_param, eye_ray, _1);

  std::vector<float> intersections(geometry.size());
  std::transform(geometry.begin(), geometry.end(), 
    intersections.begin(), intersects_eye_ray_at);
  // if the first element is nan no element will compare as less than it
  auto begin_near_it = std::find_if_not(
    intersections.cbegin(), intersections.cend(), isnanf);
  auto near_it = std::min_element(begin_near_it, intersections.cend());
  auto near_geometry_it = geometry.begin() +
    std::distance(intersections.cbegin(), near_it);
  rsi.t = *near_it;
  rsi.near_geometry_it = near_geometry_it;
  return rsi;
}

struct geometry_t {
  std::vector<sphere_t> spheres;
};

inline ray_sphere_intersect cast_ray(
  const ray_t& eye_ray,
  const geometry_t& geometry)
{
  return cast_ray(eye_ray, geometry.spheres);
}

inline vec3f reflected(const vec3f& incident, const vec3f& normal) {
  return -2 * dot(incident, normal) * normal + incident;
}

#endif
