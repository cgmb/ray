#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>
#include <yaml-cpp/yaml.h>
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

enum {
  INVALID_ARG = -1,
  SCENE_FILE_ARG,
  OUTPUT_FILE_ARG,
};

struct user_inputs {
  user_inputs()
    : scene_file(0)
    , output_file(0)
    , requests_help(false)
  {
  }

  const char* scene_file;
  const char* output_file;
  bool requests_help;
};

user_inputs parse_inputs(int argc, char** argv) {
  user_inputs in;
  int next_expected_arg = INVALID_ARG;
  for (int i = 1; i < argc; ++i) {
    if (next_expected_arg == SCENE_FILE_ARG) {
      in.scene_file = argv[i];
      next_expected_arg = INVALID_ARG;
    } else if (next_expected_arg == OUTPUT_FILE_ARG) {
      in.output_file = argv[i];
      next_expected_arg = INVALID_ARG;
    } else if (!strcmp(argv[i], "--scene")) {
      next_expected_arg = SCENE_FILE_ARG;
    } else if (!strcmp(argv[i], "--output")) {
      next_expected_arg = OUTPUT_FILE_ARG;
    } else if (!strcmp(argv[i], "--help")) {
      in.requests_help = true;
    } else {
      std::cerr << "Unknown input: " << argv[i] << std::endl;
    }
  }
  return in;
}

void print_help() {

}

struct scene {
  resolution res;

  vec3f observer;
  vec3f screen_top_left;
  vec3f screen_top_right;
  vec3f screen_bottom_right;

  std::vector<sphere> geometry;
  std::vector<light> lights;

  vec3f screen_offset_per_px_x() const {
    vec3f screen_offset_x = screen_top_right - screen_top_left;
    return (1.f / res.x) * screen_offset_x;
  }

  vec3f screen_offset_per_px_y() const {
    vec3f screen_offset_y = screen_bottom_right - screen_top_right;
    return (1.f / res.y) * screen_offset_y;
  }
};

vec3f parse_vec3f_node(const YAML::Node& node) {
  vec3f value;
  if (node.size() == 3u) {
    value[0] = node[0].as<float>();
    value[1] = node[1].as<float>();
    value[2] = node[2].as<float>();
  } else {
    std::cerr << node.Tag() << " is a vec3f, which requires 3 values, not "
      << node.size() << "." << std::endl;
    throw std::runtime_error("Incorrect number of nodes on vec3f");
  }
  return value;
}

scene load_scene_from_file(const char* scene_file) {
  scene s;

  YAML::Node config = YAML::LoadFile(scene_file);
  if (YAML::Node observer = config["observer"]) {
    s.observer = parse_vec3f_node(observer);
  } else {
    std::cerr << "Scene requires observer!" << std::endl;
  }

//  s.observer = vec3f{ 0, 0, -10 };
  s.screen_top_left = vec3f{ -5, 5, 0 };
  s.screen_top_right = vec3f{ 5, 5, 0 };
  s.screen_bottom_right = vec3f{ 5, -5, 0 };
  s.res = resolution{ 100, 100 };
  s.geometry.push_back(sphere{ vec3f(0, 0, 10), 3 });
  s.lights.push_back(light{ vec3f(0, 0, -10), vec3f(1, 1, 1) });
  return s;
}

const char* get_with_default(const char* primary, const char* fallback) {
  return primary ? primary : fallback;
}

int main(int argc, char** argv) {
  user_inputs user = parse_inputs(argc, argv);
  if (user.requests_help) {
    print_help();
    std::exit(0);
  }

  scene s = load_scene_from_file(
    get_with_default(user.scene_file, "scene.yml"));

  vec3f observer = s.observer;
  vec3f screen_top_left = s.screen_top_left;
  std::vector<sphere> geometry = s.geometry;
  std::vector<light> lights = s.lights;
  resolution res = s.res;

  vec3f screen_offset_per_px_x = s.screen_offset_per_px_x();
  vec3f screen_offset_per_px_y = s.screen_offset_per_px_y();

  image img(res.x, res.y);

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

  if (!img.save_as_png(get_with_default(user.output_file, "output.png"))) {
    return 1;
  }
  
  return 0;
}
