#include <cmath>
#include "geometry.h"
#include "vec3f.h"
#include "test/test.h"

namespace {
template<class T>
bool ray_sphere_intersect(ray_t r, sphere_t s, T expectation) {
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
    ray_t::from_point_vector(vec3f(-3,0,1), normalized(vec3f(2,1,0))),
    sphere_t::from_center_radius_squared(vec3f(1,1,1), 4),
    fuzzy_eq_vec3f(vec3f(-1,1,1), 0.25)));

RTEST(ray_miss_sphere,
  ray_sphere_intersect(
    ray_t::from_point_vector(vec3f(-3,1,1), normalized(vec3f(2,3,1))),
    sphere_t::from_center_radius_squared(vec3f(1,1,1), 4),
    is_null));

RTEST(ray_sphere_behind,
  ray_sphere_intersect(
    ray_t::from_point_vector(vec3f(0,0,0), normalized(vec3f(0,0,1))),
    sphere_t::from_center_radius_squared(vec3f(0,0,-4), 4),
    is_null));

RTEST(reflect_straight_on_z, []{
  vec3f reflect = reflected(vec3f(0,0,-1), vec3f(0,0,1));
  return magnitude(reflect - vec3f(0,0,1)) < 1e-4f;
});

RTEST(refract_straight_on_z, []{
  vec3f value = refracted(vec3f(0,0,-1), vec3f(0,0,1), 1.f, 1.f);
  return magnitude(value - vec3f(0,0,-1)) < 1e-4f;
});

RTEST(refract_angle_equal_n, []{
  vec3f incident = normalized(vec3f(0,1,-1));
  vec3f value = refracted(incident, vec3f(0,0,1), 1.f, 1.f);
  return magnitude(value - incident) < 1e-4f;
});

RTEST(refract_angle_different_n, []{
  vec3f incident = normalized(vec3f(0,1,-1));
  vec3f normal = vec3f(0,0,1);
  vec3f value = refracted(incident, normal, 1.f, 1.25f);
  return dot(-normal,value) < dot(-normal,incident);
});

} // namespace
#include "vector_debug.h"
test_results test_geometry() {
  return ray_through_sphere() % ray_miss_sphere() % ray_sphere_behind() %
    reflect_straight_on_z() % refract_straight_on_z() %
    refract_angle_equal_n() % refract_angle_different_n();
}
