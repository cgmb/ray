#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "geometry.h"
#include "vec3f.h"

struct resolution_t {
  unsigned x;
  unsigned y;
};

struct light_t {
  vec3f position;
  vec3f color;
};

struct material_t {
  vec3f color;
  float opacity;
  float refractive_index;
  float reflectivity;
};

struct scene_t {
  resolution_t res;

  vec3f observer;
  vec3f screen_top_left;
  vec3f screen_top_right;
  vec3f screen_bottom_right;

  geometry_t geometry;
  std::vector<material_t> sphere_materials;
  std::vector<light_t> lights;
  vec3f ambient_light;

  vec3f screen_offset_per_px_x() const;
  vec3f screen_offset_per_px_y() const;
};

inline vec3f scene_t::screen_offset_per_px_x() const {
  vec3f screen_offset_x = screen_top_right - screen_top_left;
  return (1.f / res.x) * screen_offset_x;
}

inline vec3f scene_t::screen_offset_per_px_y() const {
  vec3f screen_offset_y = screen_bottom_right - screen_top_right;
  return (1.f / res.y) * screen_offset_y;
}


namespace YAML { class Node; };

vec3f parse_vec3f_node(const YAML::Node& node);
resolution_t parse_resolution_node(const YAML::Node& node);

sphere_t parse_sphere_node(const YAML::Node& node);
scene_t load_scene_from_file(const char* scene_file);
scene_t try_load_scene_from_file(const char* scene_file, int error_exit_code);
scene_t generate_default_scene();

#endif
