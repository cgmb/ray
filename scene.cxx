#include <functional>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "scene.h"

using std::placeholders::_1;

namespace {

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

resolution_t parse_resolution_node(const YAML::Node& node) {
  resolution_t value;
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

sphere_t parse_sphere_node(const YAML::Node& node) {
  sphere_t value;
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

vec3f retrieve_optional_secondary_color(const YAML::Node& node) {
  vec3f value;
  if (YAML::Node color = node["secondary_color"]) {
    value = parse_vec3f_node(color);
  } else {
    value = vec3f(0, 0, 0);
  }
  return value;
}

tex3d_lookup_t retrieve_optional_texture(const YAML::Node& node,
  const vec3f& color, const vec3f& secondary_color)
  {
  tex3d_lookup_t value;
  if (YAML::Node texture = node["texture"]) {
    std::string name;
    try {
      name = texture.as<std::string>();
    } catch(const YAML::RepresentationException&) {
      if (YAML::Node texture = node["name"]) {
      } else {
        throw std::runtime_error("Texture requires name!");
      }
    }
    if (name == "checkerboard") {
      value = std::bind(algo_texture::checkerboard_3d, _1,
        color, secondary_color);;
    } else if (name == "dotsnlines") {

      float period = 1.f;
      if (YAML::Node p = node["period"]) {
        period = p.as<float>();
      }

      float width = 0.125f;
      if (YAML::Node w = node["width"]) {
        width = w.as<float>();
      }

      value = std::bind(algo_texture::dotsnlines_3d, _1, period, width,
        color, secondary_color);
    } else {
      throw std::runtime_error("Unknown texture type!");
    }
  }
  return value;
}

material_t retrieve_optional_material(const YAML::Node& node) {
  material_t value;
  value.color = retrieve_optional_color(node);
  value.secondary_color = retrieve_optional_secondary_color(node);
  value.texture = retrieve_optional_texture(node,
    value.color, value.secondary_color);

  if (YAML::Node reflectivity = node["reflectivity"]) {
    value.reflectivity = reflectivity.as<float>();
  } else if (YAML::Node mirrored = node["mirrored"]) {
    value.reflectivity = mirrored.as<bool>() ? 1.f : 0.f;
  } else {
    value.reflectivity = 0.f;
  }

  if (YAML::Node refractive_index = node["refractive_index"]) {
    value.refractive_index = refractive_index.as<float>();
  } else {
    value.refractive_index = 1.f;
  }

  if (YAML::Node opacity = node["opacity"]) {
    value.opacity = opacity.as<float>();
  } else {
    value.opacity = 1.f;
  }

  if (YAML::Node n = node["k_ambient"]) {
    value.k_ambient = n.as<float>();
  } else {
    value.k_ambient = 1.f;
  }

  if (YAML::Node n = node["k_matte"]) {
    value.k_matte = n.as<float>();
  } else {
    value.k_matte = 0.0f;
  }

  if (YAML::Node n = node["k_specular"]) {
    value.k_specular = n.as<float>();
  } else {
    value.k_specular = 0.0f;
  }

  if (YAML::Node n = node["k_specular_n"]) {
    value.k_specular_n = n.as<float>();
    if (std::floor(value.k_specular_n) != value.k_specular_n) {
      throw std::runtime_error("Fractional k_specular_n values not allowed!");
    }
  } else {
    value.k_specular_n = 2.f;
  }

  if (YAML::Node n = node["k_flat"]) {
    value.k_flat = n.as<float>();
  } else {
    value.k_flat = (value.k_matte > 0.f || value.k_specular > 0.f) ?
      0.f : 1.f;
  }

  return value;
}

light_t parse_point_light_node(const YAML::Node& node) {
  light_t value;
  if (YAML::Node position = node["position"]) {
    value.position = parse_vec3f_node(position);
  } else {
    throw std::runtime_error("Point light requires position!");
  }

  if (YAML::Node color = node["color"]) {
    value.color = parse_vec3f_node(color);
  } else {
    throw std::runtime_error("Point light requires color!");
  }
  return value;
}

/* This is not optimal in terms of memory usage but is an easy
   approximation.
*/
std::vector<light_t> parse_sphere_light_node(const YAML::Node& node) {
  std::vector<light_t> value;

  vec3f center;
  if (YAML::Node n = node["center"]) {
    center = parse_vec3f_node(n);
  } else if (YAML::Node n = node["position"]) {
    center = parse_vec3f_node(n);
  } else {
    throw std::runtime_error("Sphere light requires center!");
  }

  vec3f color;
  if (YAML::Node n = node["color"]) {
    color = parse_vec3f_node(n);
  } else {
    throw std::runtime_error("Sphere light requires color!");
  }

  float radius;
  if (YAML::Node n = node["radius"]) {
    radius = n.as<float>();
  } else {
    throw std::runtime_error("Sphere light requires radius!");
  }

  float density;
  if (YAML::Node n = node["density"]) {
    density = n.as<float>();
  } else {
    density = 1.f;
  }

  unsigned seed;
  if (YAML::Node n = node["seed"]) {
    seed = n.as<float>();
  } else {
    seed = 0u;
  }

  std::mt19937 engine(seed);
  std::uniform_real_distribution<float> distribution(0.f, 1.f);
  auto rng = std::bind(distribution, engine);

  float volume = 4.f / 3.f * M_PI * radius * radius * radius;
  size_t points_required = volume * density;
  vec3f per_point_color = color / points_required;
  while (value.size() < points_required) {
    vec3f candidate(rng(), rng(), rng());
    if (magnitude(candidate) <= 1.f) {
      light_t pl;
      pl.position = 2.f * radius * candidate + center;
      pl.color = per_point_color;
      value.push_back(pl);
    }
  }

  return value;
}

mesh_t parse_mesh_node(const YAML::Node& node) {
  std::vector<vec3f> v;
  std::vector<unsigned short> i;

  if (node["vertexes"] || node["indexes"]) {
    if (YAML::Node vertexes = node["vertexes"]) {
      for (auto it = vertexes.begin(); it != vertexes.end(); ++it) {
        v.push_back(parse_vec3f_node(*it));
      }
    } else {
      throw std::runtime_error("Inline mesh requires vertexes!");
    }

    if (YAML::Node indexes = node["indexes"]) {
      for (auto it = indexes.begin(); it != indexes.end(); ++it) {
        i.push_back(it->as<unsigned short>());
      }
    } else { 
      unsigned short auto_index = 0u;
      for (auto it = v.begin(); it != v.end(); ++it) {
        i.push_back(auto_index++);
      }
    }
  } else if (YAML::Node file = node["file"]) {
    throw std::runtime_error("External mesh files not supported (yet)!");
  } else {
    throw std::runtime_error("Mesh requires vertexes!");
  }

  bool smooth;
  if (YAML::Node n = node["smooth"]) {
    smooth = n.as<bool>();
  } else {
    smooth = false;
  }

  return mesh_t(v, i, smooth);
}

} // namespace

scene_t load_scene_from_file(const char* scene_file) {
  scene_t s;
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

  if (YAML::Node samples = config["samples"]) {
    s.sample_count = samples.as<unsigned>();
  } else {
    s.sample_count = 1u;
  }

  if (YAML::Node geometry = config["geometry"]) {
    if (YAML::Node spheres = geometry["spheres"]) {
      for (auto it = spheres.begin(); it != spheres.end(); ++it) {
        s.geometry.spheres.push_back(parse_sphere_node(*it));
        s.sphere_materials.push_back(retrieve_optional_material(*it));
      }
    }

    if (YAML::Node meshes = geometry["meshes"]) {
      for (auto it = meshes.begin(); it != meshes.end(); ++it) {
        s.geometry.meshes.push_back(parse_mesh_node(*it));
        s.mesh_materials.push_back(retrieve_optional_material(*it));
      }
    }
  } else {
    throw std::runtime_error("Scene requires geometry!");
  }

  if (YAML::Node lights = config["lights"]) {
    if (YAML::Node ambient = lights["ambient"]) {
      s.ambient_light = parse_vec3f_node(ambient);
    } else {
      s.ambient_light = vec3f(0,0,0);
    }
    if (YAML::Node points = lights["points"]) {
      for (auto it = points.begin(); it != points.end(); ++it) {
        s.lights.push_back(parse_point_light_node(*it));
      }
    }
    if (YAML::Node spheres = lights["spheres"]) {
      for (auto it = spheres.begin(); it != spheres.end(); ++it) {
        std::vector<light_t> samples = parse_sphere_light_node(*it);
        s.lights.insert(s.lights.end(), samples.begin(), samples.end());
      }
    }
  } else {
    throw std::runtime_error("Scene requires lights!");
  }

  return s;
}

scene_t try_load_scene_from_file(const char* scene_file, int error_exit_code) {
  try {
    return load_scene_from_file(scene_file);
  } catch (const std::exception& e) {
    std::cerr << "Failed to load " << scene_file << "\nEncountered error:\n"
      << e.what() << std::endl;
    std::exit(error_exit_code);
  }
}
