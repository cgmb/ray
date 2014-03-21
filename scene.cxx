#include <yaml-cpp/yaml.h>
#include "scene.h"

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

resolution parse_resolution_node(const YAML::Node& node) {
  resolution value;
  if (node.size() == 2u) {
    value.x = node[0].as<float>();
    value.y = node[1].as<float>();
  } else {
    std::cerr << node.Tag() << " is a resolution which requires 2 values, not "
      << node.size() << "." << std::endl;
    throw std::runtime_error("Incorrect number of nodes on resolution");
  }
  return value;
}

sphere parse_sphere_node(const YAML::Node& node) {
  sphere value;
  if (YAML::Node center = node["center"]) {
    value.center = parse_vec3f_node(center);
  } else {
    throw std::runtime_error("Sphere requires center!");
  }

  if (YAML::Node radius = node["radius"]) {
    float r = radius.as<float>();
    value.radius_squared = r * r;
  } else {
    throw std::runtime_error("Sphere requires radius!");
  }

  return value;
}

vec3f retrieve_optional_color(const YAML::Node& node) {
  vec3f value;
  if (YAML::Node color = node["color"]) {
    value = parse_vec3f_node(color);
  } else {
    value = vec3f(1, 1, 1);
  }
  return value;
}

scene load_scene_from_file(const char* scene_file) {
  scene s;
  YAML::Node config = YAML::LoadFile(scene_file);
  if (YAML::Node observer = config["observer"]) {
    s.observer = parse_vec3f_node(observer);
  } else {
    throw std::runtime_error("Scene requires observer!");
  }

  if (YAML::Node screen = config["screen"]) {
    if (YAML::Node top_left = screen["top_left"]) {
      s.screen_top_left = parse_vec3f_node(top_left);
    } else {
      throw std::runtime_error("Screen requires top left!");
    }

    if (YAML::Node top_right = screen["top_right"]) {
      s.screen_top_right = parse_vec3f_node(top_right);
    } else {
      throw std::runtime_error("Screen requires top right!");
    }

    if (YAML::Node bottom_right = screen["bottom_right"]) {
      s.screen_bottom_right = parse_vec3f_node(bottom_right);
    } else {
      throw std::runtime_error("Screen requires bottom right!");
    }
  } else {
    throw std::runtime_error("Scene requires screen!");
  }

  if (YAML::Node resolution = config["resolution"]) {
    s.res = parse_resolution_node(resolution);
  } else {
    throw std::runtime_error("Scene requires resolution!");
  }

  if (YAML::Node geometry = config["geometry"]) {
    if (YAML::Node spheres = geometry["spheres"]) {
      for (auto it = spheres.begin(); it != spheres.end(); ++it) {
        s.spheres.push_back(parse_sphere_node(*it));
        s.sphere_colors.push_back(retrieve_optional_color(*it));
      }
    }
  } else {
    throw std::runtime_error("Scene requires geometry!");
  }
  return s;
}

scene try_load_scene_from_file(const char* scene_file, int error_exit_code) {
  try {
    return load_scene_from_file(scene_file);
  } catch (const std::exception& e) {
    std::cerr << "Failed to load " << scene_file << "\nEncountered error:\n"
      << e.what() << std::endl;
    std::exit(error_exit_code);
  }
}

scene generate_default_scene() {
  scene s;
  s.observer = vec3f{ 0, 0, -10 };
  s.screen_top_left = vec3f{ -5, 5, 0 };
  s.screen_top_right = vec3f{ 5, 5, 0 };
  s.screen_bottom_right = vec3f{ 5, -5, 0 };
  s.res = resolution{ 100, 100 };
  s.spheres.push_back(sphere{ vec3f(0, 0, 10), 3 });
  s.lights.push_back(light{ vec3f(0, 0, -10), vec3f(1, 1, 1) });
  return s;
}
