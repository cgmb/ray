#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "geometry.h"
#include "vec3f.h"

struct resolution {
  unsigned x;
  unsigned y;
};

struct light {
  vec3f position;
  vec3f color;
};

struct scene {
  resolution res;

  vec3f observer;
  vec3f screen_top_left;
  vec3f screen_top_right;
  vec3f screen_bottom_right;

  std::vector<sphere> geometry;
  std::vector<light> lights;

  vec3f screen_offset_per_px_x() const;
  vec3f screen_offset_per_px_y() const;
};

inline vec3f scene::screen_offset_per_px_x() const {
  vec3f screen_offset_x = screen_top_right - screen_top_left;
  return (1.f / res.x) * screen_offset_x;
}

inline vec3f scene::screen_offset_per_px_y() const {
  vec3f screen_offset_y = screen_bottom_right - screen_top_right;
  return (1.f / res.y) * screen_offset_y;
}


namespace YAML { class Node; };

vec3f parse_vec3f_node(const YAML::Node& node);
resolution parse_resolution_node(const YAML::Node& node);

sphere parse_sphere_node(const YAML::Node& node);
scene load_scene_from_file(const char* scene_file);
scene try_load_scene_from_file(const char* scene_file, int error_exit_code);
scene generate_default_scene();

#endif
