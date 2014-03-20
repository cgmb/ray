#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>
#include "vec3f.h"

struct ray {
  static ray from_point_vector(const vec3f& start, const vec3f& direction) {
    ray r = { start, direction };
    return r;
  }

  // returns the position of the ray at t multiples of the ray direction.
  vec3f position_at(float t) const {
    return start + (direction * t);
  }

  vec3f start;
  vec3f direction;
};

struct sphere {
  static sphere from_center_radius_squared(const vec3f& center, float radius_squared) {
    sphere s = { center, radius_squared };
    return s;
  }

  vec3f center;
  float radius_squared;
};

// todo: move to more appropriate header
inline bool abs_fuzzy_eq(double lhs, double rhs, double abs_epsilon) {
  return std::abs(lhs - rhs) < abs_epsilon;
}

// returns the t value for the near intersect point
// along the parametric equation of the ray (pos = origin + direction * t)
inline float near_intersect_param(const ray& r, const sphere& s) {
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
    return std::numeric_limits<float>::quiet_NaN();
  } else if (x1 < 0.f) {
    return x2;
  } else {
    return x1;
  }
}

inline vec3f near_intersect(const ray& r, const sphere& s) {
  return r.start + (near_intersect_param(r, s) * r.direction);
}

#endif
