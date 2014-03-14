#include <cmath>
#include "geometry.h"
#include "vec3f.h"
#include "test/test.h"

namespace {
template<class T>
bool ray_sphere_intersect(ray r, sphere s, T expectation) {
  return expectation(near_intersect(r, s));
}

// test comparisons
bool is_null(const vec3f& v) {
  return 
    std::isnan(v[0]) &&
    std::isnan(v[1]) &&
    std::isnan(v[2]);
}

struct fuzzy_eq_vec3f {
  fuzzy_eq_vec3f(const vec3f& lhs, float threshold)
    : lhs_(lhs)
    , threshold_(threshold)
  {
  }

  bool operator()(const vec3f& rhs) const {
    return magnitude(lhs_ - rhs) < threshold_;
  }

private:
  vec3f lhs_;
  float threshold_;
};

// tests
RTEST(ray_through_sphere,
  ray_sphere_intersect(
    ray::from_point_vector(vec3f(-3,0,1), normalized(vec3f(2,1,0))),
    sphere::from_center_radius_squared(vec3f(1,1,1), 4),
    fuzzy_eq_vec3f(vec3f(-1,1,1), 0.25)));

RTEST(ray_miss_sphere,
  ray_sphere_intersect(
    ray::from_point_vector(vec3f(-3,1,1), normalized(vec3f(2,3,1))),
    sphere::from_center_radius_squared(vec3f(1,1,1), 4),
    is_null));
} // namespace

test_results test_geometry() {
  return ray_through_sphere() % ray_miss_sphere();
}
