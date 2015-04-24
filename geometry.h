#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <functional>
#include <limits>
#include <vector>
#include "vec3f.h"

using std::placeholders::_1;

/* ray- a representation of a line starting at some point
*/
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

/* sphere - a representation of a perfect sphere
*/
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

inline vec3f triangle_normal(
  const vec3f& a, const vec3f& b, const vec3f& c)
{
  vec3f ab = b - a;
  vec3f ac = c - a;
  return normalized(cross(ab, ac));
}

template<class iterator>
sphere_t get_bounding_sphere(iterator begin, iterator end) {
  vec3f min(FLT_MAX, FLT_MAX, FLT_MAX);
  vec3f max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  for (; begin != end; ++begin) {
    const vec3f value = *begin;
    for (size_t i=0u; i<3u; ++i) {
      if (value[i] < min[i]) {
        min[i] = value[i];
      } else if (value[i] > max[i]) {
        max[i] = value[i];
      }
    }
  }
  vec3f center = min/2.f + max/2.f;
  float radius = std::max({max[0]-min[0], max[1]-min[1], max[2]-min[2]});
  return sphere_t::from_center_radius_squared(center, radius * radius);
}

/* mesh - a representation of an indexed triangle mesh
*/
struct mesh_t {
  mesh_t()
    : bounding_sphere(sphere_t{vec3f(0,0,0), 0})
    , smooth(false)
  {
  }

  mesh_t(
    const std::vector<vec3f>& vertexes,
    const std::vector<unsigned int>& indexes,
    bool smooth = false)
    : vertexes(vertexes)
    , indexes(indexes)
    , vertex_normals(vertexes.size())
    , face_normals(indexes.size() / 3u)
    , smooth(smooth)
  {
    assert(vertexes.size() <= std::numeric_limits<unsigned int>::max());
    calculate_normals();
    calculate_bounding_sphere();
  }

  void calculate_normals() {
    // calculate face normals and collect data for vertex normals
    // also calculate face centers
    std::fill(vertex_normals.begin(), vertex_normals.end(), vec3f(0,0,0));
    for (size_t i = 0u; i < face_normals.size(); ++i) {
      unsigned int i1 = indexes[3*i];
      unsigned int i2 = indexes[3*i + 1];
      unsigned int i3 = indexes[3*i + 2];

      vec3f normal = -triangle_normal(
        vertexes[i1], vertexes[i2], vertexes[i3]);
      face_normals[i] = normal;

      // associate this face normal with each vertex
      vertex_normals[i1] += normal;
      vertex_normals[i2] += normal;
      vertex_normals[i3] += normal;
    }

    std::transform(vertex_normals.begin(), vertex_normals.end(),
      vertex_normals.begin(), normalized);
  }

  void calculate_bounding_sphere() {
    bounding_sphere = get_bounding_sphere(vertexes.cbegin(), vertexes.cend());
  }

  std::vector<vec3f> vertexes;
  std::vector<unsigned int> indexes;
  std::vector<vec3f> vertex_normals;
  std::vector<vec3f> face_normals;
  sphere_t bounding_sphere;
  bool smooth;
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

/* Information about a collision between a ray and a vector of spheres
*/
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

inline ray_sphere_intersect get_ray_sphere_intersect(
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
  if (begin_near_it == intersections.cend()) {
    return rsi;
  }
  auto near_it = std::min_element(begin_near_it, intersections.cend());
  auto near_geometry_it = geometry.begin() +
    std::distance(intersections.cbegin(), near_it);
  rsi.t = *near_it;
  rsi.near_geometry_it = near_geometry_it;
  return rsi;
}

/* Information about a collision between a ray and a triangle
*/
struct ray_triangle_intersect {
  float t;
  size_t near_face_index;
};

/* Intersects are ordered by their t value
   Note that lists containing non-existant intersects are only
   partially ordered. Non-existant intersects have no order.
*/
inline bool operator<(const ray_triangle_intersect& lhs,
  const ray_triangle_intersect& rhs)
{
  return lhs.t < rhs.t;
}

/* Intersect does not exist if t is not a number
*/
inline bool intersect_exists(const ray_triangle_intersect& value) {
  return !std::isnan(value.t);
}

/* Returns the nearest intersect point
   along the parametric equation of the ray (pos = origin + direction * t)
*/
inline ray_triangle_intersect get_ray_triangle_intersect(
  const ray_t& r, const mesh_t& m)
{
  assert(abs_fuzzy_eq(magnitude(r.direction), 1, 1e-3));

  std::vector<ray_triangle_intersect> intersects;
  for (size_t i = 0u; i < m.face_normals.size(); ++i) {
    vec3f v1 = m.vertexes[m.indexes[3*i]];
    vec3f v2 = m.vertexes[m.indexes[3*i + 1]];
    vec3f v3 = m.vertexes[m.indexes[3*i + 2]];

    vec3f normal = m.face_normals[i];
    float d = dot(r.direction, normal);
    if (d == 0.f) {
      continue;
    }
    float plane_intersect = -dot(r.start - v1, normal) / d;
    if (plane_intersect < 0.f) {
      continue;
    }

    vec3f point = r.position_at(plane_intersect);
    bool side_a = dot(normal, cross(v2-v1, point-v1)) < 0.f;
    bool side_b = dot(normal, cross(v3-v2, point-v2)) < 0.f;
    bool side_c = dot(normal, cross(v1-v3, point-v3)) < 0.f;

    if (side_a == side_b && side_b == side_c) {
      intersects.push_back(ray_triangle_intersect{ plane_intersect, i });
    }
  }

  auto near_it = std::min_element(intersects.begin(), intersects.end());
  if (near_it == intersects.end()) {
    return ray_triangle_intersect{ quiet_nan(), 0u };
  } else {
    return *near_it;
  }
}

inline bool could_ray_intersect_mesh(const ray_t& r, const mesh_t& m) {
  return !std::isnan(near_intersect_param(r, m.bounding_sphere));
}

/* Do a bounding sphere check first to filter out obvious misses
*/
inline ray_triangle_intersect get_possible_ray_triangle_intersect(
  const ray_t& r, const mesh_t& m)
{
  if (could_ray_intersect_mesh(r, m)) {
    return get_ray_triangle_intersect(r, m);
  } else {
    return ray_triangle_intersect{ quiet_nan(), 0u };
  }
}

/* Information about the intersect between a ray and a list of triangle meshes
*/
struct ray_mesh_intersect {
  float t;
  size_t near_face_index;
  std::vector<mesh_t>::const_iterator near_geometry_it;

  bool intersect_exists(const std::vector<mesh_t>& m) const {
    return !std::isnan(t) && m.end() != near_geometry_it;
  }

  size_t index_in(const std::vector<mesh_t>& m) const {
    return std::distance(m.begin(), near_geometry_it);
  }

  // todo: move to mesh_t
  vec3f get_normal_at(const vec3f& pos) const {
    if (near_geometry_it->smooth) {

      // use barycentric coordinates.
      // we probably should have used those for intersection tests
      unsigned int i1 = near_geometry_it->indexes[3u * near_face_index];
      unsigned int i2 = near_geometry_it->indexes[3u * near_face_index + 1];
      unsigned int i3 = near_geometry_it->indexes[3u * near_face_index + 2];
      vec3f v1 = near_geometry_it->vertexes[i1];
      vec3f v2 = near_geometry_it->vertexes[i2];
      vec3f v3 = near_geometry_it->vertexes[i3];
      vec3f n1 = near_geometry_it->vertex_normals[i1];
      vec3f n2 = near_geometry_it->vertex_normals[i2];
      vec3f n3 = near_geometry_it->vertex_normals[i3];

      float area = 0.5f * magnitude(cross(v2-v1, v3-v1));
      vec3f v1pos = pos-v1;
      float u = 0.5f * magnitude(cross(v1pos, v3-v1)) / area;
      float v = 0.5f * magnitude(cross(v1pos, v2-v1)) / area;
      float w = 1.f - u - v;

      vec3f n = (w*n1 + u*n2 + v*n3);
      return normalized(n);
    } else {
      return near_geometry_it->face_normals[near_face_index];
    }
  }
};

inline ray_mesh_intersect get_ray_mesh_intersect(
  const ray_t& eye_ray,
  const std::vector<mesh_t>& geometry)
{
  ray_mesh_intersect rmi = { quiet_nan(), 0u, geometry.end() };
  if (geometry.empty()) {
    return rmi;
  }

  std::vector<ray_triangle_intersect> intersections(geometry.size());
  auto intersects_eye_ray_at =
    std::bind(get_possible_ray_triangle_intersect, eye_ray, _1);
  std::transform(geometry.begin(), geometry.end(), 
    intersections.begin(), intersects_eye_ray_at);
  // if the first element is nan no element will compare as less than it
  auto begin_near_it = std::find_if(
    intersections.cbegin(), intersections.cend(), intersect_exists);
  if (begin_near_it == intersections.cend()) {
    return rmi;
  }
  auto near_it = std::min_element(begin_near_it, intersections.cend());

  auto near_geometry_it = geometry.begin() +
    std::distance(intersections.cbegin(), near_it);

  rmi.t = near_it->t;
  rmi.near_face_index = near_it->near_face_index;
  rmi.near_geometry_it = near_geometry_it;
  return rmi;
}

/* A collection of 3D shapes
*/
struct geometry_t {
  std::vector<sphere_t> spheres;
  std::vector<mesh_t> meshes;
};

inline vec3f reflected(const vec3f& incident, const vec3f& normal) {
  return incident -2.f * dot(incident, normal) * normal;
}

inline vec3f refracted(const vec3f& incident, const vec3f& normal,
  float n1, float n2)
{
  float dot_in = dot(incident, normal);
  float dot_in_sq = dot_in * dot_in;

  float n1_n2 = n1 / n2;
  float n1_n2_sq = n1_n2 * n1_n2;

  return n1_n2 * (incident - dot_in * normal) -
    normal * sqrt(1.f - n1_n2_sq * (1.f - dot_in_sq));
}

#endif
