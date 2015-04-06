#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>
#include "geometry.h"
#include "help_text.h"
#include "image.h"
#include "scene.h"
#include "vec3f.h"

enum {
  EXIT_OK = 0,
  EXIT_FAIL_SAVE,
  EXIT_FAIL_LOAD,
  EXIT_BAD_ARGS,
};

enum {
  INVALID_ARG = -1,
  HELP_ARG,
  SCENE_FILE_ARG,
  OUTPUT_FILE_ARG,
  THREAD_COUNT_ARG,
};

struct user_inputs {
  user_inputs()
    : scene_file(0)
    , output_file(0)
    , thread_count(1)
    , requests_help(false)
    , requests_help_scene(false)
    , display_progress(false)
  {
  }

  const char* scene_file;
  const char* output_file;
  unsigned thread_count;
  bool requests_help;
  bool requests_help_scene;
  bool display_progress;
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
    } else if (next_expected_arg == THREAD_COUNT_ARG) {
      std::istringstream input(argv[i]);
      input >> in.thread_count;
      if (!input) {
        std::cerr << "Invalid thread count: " << argv[i] << std::endl;
        std::exit(EXIT_BAD_ARGS);
      }
      next_expected_arg = INVALID_ARG;
    } else if (!strcmp(argv[i], "--scene")) {
      next_expected_arg = SCENE_FILE_ARG;
    } else if (!strcmp(argv[i], "--output")) {
      next_expected_arg = OUTPUT_FILE_ARG;
    } else if (!strcmp(argv[i], "--threads") || !strcmp(argv[i], "-j")) {
      next_expected_arg = THREAD_COUNT_ARG;
    } else if (!strcmp(argv[i], "--progress")) {
      in.display_progress = true;;
    } else if (!strcmp(argv[i], "--help")) {
      in.requests_help = true;
      next_expected_arg = HELP_ARG;
    } else if (next_expected_arg == HELP_ARG) {
      if (!strcmp(argv[i], "scene")) {
        in.requests_help_scene = true;
        in.requests_help = false;
      }
      next_expected_arg = INVALID_ARG;
    } else {
      std::cerr << "Unrecognized input: " << argv[i] << std::endl;
      std::exit(EXIT_BAD_ARGS);
    }
  }
  return in;
}

const char* get_with_default(const char* primary, const char* fallback) {
  return primary ? primary : fallback;
}

enum nearest_t {
  NOTHING_NEAREST,
  SPHERE_NEAREST,
  MESH_NEAREST,
};

nearest_t nearest_intersect(
  const ray_sphere_intersect& rsi,
  const std::vector<sphere_t>& spheres,
  const ray_mesh_intersect& rmi,
  const std::vector<mesh_t>& meshes)
{
  if (rsi.intersect_exists(spheres)) {
    if (rmi.intersect_exists(meshes)) {
      return rsi.t < rmi.t ? SPHERE_NEAREST : MESH_NEAREST;
    } else {
      return SPHERE_NEAREST;
    }
  } else {
    if (rmi.intersect_exists(meshes)) {
      return MESH_NEAREST;
    } else {
      return NOTHING_NEAREST;
    }
  }
}

enum cast_policy_t {
  CAST_TO_OBJECT,
  CAST_TO_LIGHT,
};

// The maximum recursive depth
const unsigned MAX_RECURSE = 10u;

// The distance along the ray that the collision is offset by,
// to ensure floating point inaccuracy doesn't result in recollision
// with the same surface upon recasting from the collision point.
// Overall, it's a bit of a hack. A backoff algorithm would be more
// appropriate.
const float BACKOFF = 1e-3f;

// takes two random numbers [0..1] and returns a random direction
//vec3f random_direction(float x1, float x2) {
//  const float TWO_PI = 2 * M_PI;
//  x2 = 2 * x2 - 1; // [-1..1]
//  return vec3f(sin(TWO_PI * x1), cos(TWO_PI * x1), 2*asin(x2)/M_PI);
//}

template<class T>
vec3f random_direction(T& rng) {
  vec3f v;
  float m2;
  do {
    v = vec3f(rng(), rng(), rng());
    m2 = v.x()*v.x() + v.y()*v.y() + v.z()*v.z();
  } while(m2 > 1);
  return v / sqrt(m2);
}

template<class T>
vec3f random_downward_direction(T& rng) {
  vec3f v = random_direction(rng);
  if (v.y() > 0) {
    v[1] = -v[1];
  }
  return v;
}

struct photon_hit {
  vec3f position;
  vec3f direction;
  vec3f color;
};

struct photon_hits {
  photon_hits() = default;
  photon_hits(size_t sphere_count, size_t mesh_count)
    : sphere_hits(sphere_count)
    , mesh_hits(mesh_count)
  {}

  std::vector<std::vector<photon_hit>> sphere_hits;
  std::vector<std::vector<photon_hit>> mesh_hits;
};

photon_hits g_photon_hits;

// each object gets a vector of photons with a location, direction and color
void add_to_sphere_photon_map(size_t obj_idx,
  const vec3f& position, const vec3f& direction, const vec3f& energy) {
  g_photon_hits.sphere_hits[obj_idx].push_back(
    photon_hit{position, direction, energy});
}

void add_to_mesh_photon_map(size_t obj_idx,
  const vec3f& position, const vec3f& direction, const vec3f& energy) {
  g_photon_hits.mesh_hits[obj_idx].push_back(
    photon_hit{position, direction, energy});
}

void map_photon(const ray_t& ray, const scene_t& s, const vec3f& energy,
  float refractive_index, bool indirect, unsigned int recursion_depth) {
  ray_sphere_intersect rsi = get_ray_sphere_intersect(ray, s.geometry.spheres);
  ray_mesh_intersect rmi = get_ray_mesh_intersect(ray, s.geometry.meshes);

  nearest_t nearest =
    nearest_intersect(rsi, s.geometry.spheres, rmi, s.geometry.meshes);
  if (nearest == SPHERE_NEAREST) {
    size_t sphere_idx = rsi.index_in(s.geometry.spheres);
    material_t material = s.sphere_materials[sphere_idx];
    if(material.opacity < 1.f) {
      vec3f inside_pos = ray.position_at(rsi.t + BACKOFF);
      vec3f normal = rsi.near_geometry_it->normal_at(inside_pos);
      if (dot(ray.direction, normal) > 0.f) {
        normal = -normal;
      }
      ray_t refracted_ray = { inside_pos, refracted(ray.direction, normal,
        refractive_index, material.refractive_index) };
      if (recursion_depth < MAX_RECURSE) {
        // todo: handle refractive index for leaving a volume
        map_photon(refracted_ray, s, energy, material.refractive_index, true,
          recursion_depth + 1u);
      } else {
        std::cerr << "Hit max recurse depth!" << std::endl;
      }
    // todo: handle reflective
    } else if (indirect) {
      add_to_sphere_photon_map(sphere_idx, ray.position_at(rsi.t), ray.direction, energy);
    }
  } else if (nearest == MESH_NEAREST) {
    size_t mesh_idx = rmi.index_in(s.geometry.meshes);
    material_t material = s.mesh_materials[mesh_idx];
    if (material.opacity < 1.f) {
      vec3f inside_pos = ray.position_at(rsi.t + BACKOFF);
      vec3f normal = rmi.get_normal_at(inside_pos);
      if (dot(ray.direction, normal) > 0.f) {
        normal = -normal;
      }
      ray_t refracted_ray = { inside_pos, refracted(ray.direction, normal,
        refractive_index, material.refractive_index) };
      if (recursion_depth < MAX_RECURSE) {
        map_photon(refracted_ray, s, energy, material.refractive_index, true,
          recursion_depth + 1u);
      } else {
        std::cerr << "Hit max recurse depth!" << std::endl;
      }
    } else if (indirect) {
      add_to_mesh_photon_map(mesh_idx, ray.position_at(rmi.t), ray.direction, energy);
    }
  }
}

void create_photon_map(const scene_t& s) {
  std::mt19937 engine(123456789u);
  std::uniform_real_distribution<float> distribution(-1.f, 1.f);
  auto rng = std::bind(distribution, engine);
  g_photon_hits = photon_hits(s.geometry.spheres.size(), s.geometry.meshes.size());

  float intensity = 10000;
  for (const light_t& light : s.lights) {
    size_t samples = 10000u;
    for (size_t i = 0; i < samples; ++i) {
      // todo: sample only towards surfaces
      ray_t ray = { light.position, random_downward_direction(rng) };
      float refractive_index = 1.f;
      unsigned int recursion_depth = 0u;
      bool indirect = false;
      vec3f energy = vec3f{1.f,1.f,1.f} * intensity / samples;
      map_photon(ray, s, energy, refractive_index, indirect, recursion_depth);
    }
  }
  std::cout << g_photon_hits.sphere_hits.size() << std::endl;
  std::cout << g_photon_hits.mesh_hits.size() << std::endl;
  for (auto&& v : g_photon_hits.sphere_hits) {
    std::cout << v.size() << std::endl;
  }
  for (auto&& v : g_photon_hits.mesh_hits) {
    std::cout << v.size() << std::endl;
  }
}

float matte(vec3f normal, vec3f to_light) {
  return std::max(dot(normal, to_light), 0.f);
}

/* Casts a ray into the scene and returns a color.

  The default_color is the color returned if no object is hit.

  There are two possible cast_policy values, corresponding with
  two main types of casts:
  - cast-to-object:
    intersection with a solid object results in a search for visible lights
  - cast-to-light:
    intersection with a solid object results in the conclusion that the point
    is shadowed

  Note that these casts do not account for indirect lighting,
  i.e. global illumination
*/
vec3f cast_ray(const ray_t& ray,
  const scene_t& s,
  vec3f default_color,
  cast_policy_t cast_policy,
  float refractive_index,
  unsigned recursion_depth)
{
  vec3f color = default_color;

  ray_sphere_intersect rsi = get_ray_sphere_intersect(ray, s.geometry.spheres);
  ray_mesh_intersect rmi = get_ray_mesh_intersect(ray, s.geometry.meshes);

  nearest_t nearest =
    nearest_intersect(rsi, s.geometry.spheres, rmi, s.geometry.meshes);

  if (nearest == SPHERE_NEAREST) {
    material_t material = s.sphere_materials[rsi.index_in(s.geometry.spheres)];
    float solid_component = material.opacity - material.reflectivity;
    if (cast_policy == CAST_TO_OBJECT) {
      vec3f pos = ray.position_at(rsi.t - BACKOFF);
      vec3f material_color = material.texture ?
        material.texture(pos) : material.color;

      if (solid_component > 0.f) {
        vec3f light_color(0,0,0);
        for (const light_t& light : s.lights) {
          ray_t light_ray = { pos, normalized(light.position - pos) };
          vec3f one_light_color = cast_ray(light_ray, s, light.color,
            CAST_TO_LIGHT, refractive_index, recursion_depth + 1u);
          if (one_light_color != vec3f(0,0,0) &&
            (material.k_matte > 0.f || material.k_specular > 0.f))
          {
            // phong shading
            vec3f normal = rsi.near_geometry_it->normal_at(pos);
            float matte_light = //matte(normalized(normal), light_ray.direction);
              std::max(dot(normalized(normal), light_ray.direction), 0.f);
            float specular_light = std::pow(std::max(
              dot(ray.direction, reflected(light_ray.direction, normalized(normal))),
              0.f), material.k_specular_n);
            light_color += one_light_color *
              (material.k_matte * matte_light +
              material.k_specular * specular_light);
          } else if (one_light_color == vec3f(0,0,0)) {
            // check photon map
            vec3f intersect = ray.position_at(rsi.t);
            // todo: do we need to account for the side we're on?
            vec3f normal = normalized(rsi.near_geometry_it->normal_at(intersect));
            size_t sphere_idx = rsi.index_in(s.geometry.spheres);
            for (const photon_hit& photon : g_photon_hits.sphere_hits[sphere_idx]) {
              float dist = magnitude(photon.position - intersect);
              if (dist < 1.f) {
                one_light_color += photon.color *
                  sqrt(1.f - dist) * matte(normal, -photon.direction);
              }
            }
          }
          // normal / observer independant lighting
          // it's fast and looks nice for some things
          light_color += material.k_flat * one_light_color;
        }
        // add the combined flat/specular/matte lights wih ambient light
        color += solid_component * material_color * light_color;
        color += solid_component * material_color * material.k_ambient * s.ambient_light;
      }

      if (material.reflectivity > 0.f) {
        vec3f normal = rsi.near_geometry_it->normal_at(pos);
        ray_t reflected_ray = { pos, reflected(ray.direction, normal) };
        if (recursion_depth < MAX_RECURSE) {
          color += material.reflectivity * material.color *
            cast_ray(reflected_ray, s, default_color, cast_policy,
            refractive_index, recursion_depth + 1u);
        }
      }

      float translucence = 1.f - material.opacity;
      if (translucence > 0.f) {
        vec3f inside_pos = ray.position_at(rsi.t + BACKOFF);
        vec3f normal = rsi.near_geometry_it->normal_at(inside_pos);
        if (dot(ray.direction, normal) > 0.f) {
          normal = -normal;
        }
        ray_t refracted_ray = { inside_pos, refracted(ray.direction, normal,
          refractive_index, material.refractive_index) };
        if (recursion_depth < MAX_RECURSE) {
          color += translucence * material.color * cast_ray(refracted_ray, s,
            default_color, cast_policy, material.refractive_index,
            recursion_depth + 1u);
        } else {
          std::cerr << "Hit max recurse depth!" << std::endl;
        }
      }

    } else { // cast_policy == CAST_TO_LIGHT
      // We hit an object while looking for our light.
      // That means we're shadowed...
      color = vec3f(0,0,0);
      // unless we have caustics!
    }
  } else if (nearest == MESH_NEAREST) {

    // todo: reduce duplication between sphere and mesh color calculations

    material_t material = s.mesh_materials[rmi.index_in(s.geometry.meshes)];
    float solid_component = material.opacity - material.reflectivity;
    if (cast_policy == CAST_TO_OBJECT) {
      vec3f pos = ray.position_at(rmi.t - BACKOFF);
      vec3f material_color = material.texture ?
        material.texture(pos) : material.color;

      if (solid_component > 0.f) {
        vec3f light_color(0,0,0);
        for (const light_t& light : s.lights) {
          ray_t light_ray = { pos, normalized(light.position - pos) };
          vec3f one_light_color = cast_ray(light_ray, s, light.color,
            CAST_TO_LIGHT, refractive_index, recursion_depth + 1u);
          if (one_light_color != vec3f(0,0,0) &&
            (material.k_matte > 0.f || material.k_specular > 0.f))
          {
            // phong shading
            vec3f normal = rmi.get_normal_at(pos);
            float matte_light =
              std::max(dot(normal, light_ray.direction), 0.f);
            float specular_light = std::max(std::pow(
              dot(ray.direction, reflected(-ray.direction, normal)),
              material.k_specular_n), 0.f);
            light_color += one_light_color *
              (material.k_matte * matte_light +
              material.k_specular * specular_light);
          } else if (one_light_color == vec3f(0,0,0)) {
            // check photon map
            vec3f intersect = ray.position_at(rmi.t);
            vec3f normal = normalized(rmi.get_normal_at(intersect));
            size_t mesh_idx = rmi.index_in(s.geometry.meshes);
            for (const photon_hit& photon : g_photon_hits.mesh_hits[mesh_idx]) {
              float dist = magnitude(photon.position - intersect);
              if (dist < 1.f) {
                one_light_color += photon.color *
                  sqrt(1.f - dist) * matte(normal, -photon.direction);
              }
            }
          }
          // normal / observer independant lighting
          // it's fast and looks nice for some things
          light_color += material.k_flat * one_light_color;
        }
        color += solid_component * material_color * light_color;
        color += solid_component * material_color * material.k_ambient * s.ambient_light;
      }

      if (material.reflectivity > 0.f) {
        vec3f normal = rmi.get_normal_at(pos);
        ray_t reflected_ray = { pos, reflected(ray.direction, normal) };
        if (recursion_depth < MAX_RECURSE) {
          color += material.reflectivity * material.color *
            cast_ray(reflected_ray, s, default_color, cast_policy,
            refractive_index, recursion_depth + 1u);
        }
      }

      float translucence = 1.f - material.opacity;
      if (translucence > 0.f) {
        vec3f inside_pos = ray.position_at(rsi.t + BACKOFF);
        vec3f normal = rmi.get_normal_at(inside_pos);
        if (dot(ray.direction, normal) > 0.f) {
          normal = -normal;
        }
        ray_t refracted_ray = { inside_pos, refracted(ray.direction, normal,
          refractive_index, material.refractive_index) };
        if (recursion_depth < MAX_RECURSE) {
          color += translucence * material.color * cast_ray(refracted_ray, s,
            default_color, cast_policy, material.refractive_index,
            recursion_depth + 1u);
        } else {
          std::cerr << "Hit max recurse depth!" << std::endl;
        }
      }

    } else { // cast_policy == CAST_TO_LIGHT
      // shadowed...
      color = vec3f(0,0,0);
    }

  }
  return color;
}

void generate_pixels(unsigned thread_id,
  unsigned thread_count,
  const scene_t& s,
  const vec3f& screen_offset_per_px_x,
  const vec3f& screen_offset_per_px_y,
  bool display_progress,
  image& img)
{
  for (unsigned y = thread_id; y < s.res.y; y += thread_count) {
    std::mt19937 engine(y);
    std::uniform_real_distribution<float> distribution(0.f, 1.f);
    auto rng = std::bind(distribution, engine);
    for (unsigned x = 0u; x < s.res.x; ++x) {
      vec3f px_color = { 0, 0, 0 };
      for (unsigned sample = 0u; sample < s.sample_count; ++sample) {
        vec3f background_color = { 0, 0, 0 };
        vec3f pixel_pos = s.screen_top_left +
          (x + rng()) * screen_offset_per_px_x +
          (y + rng()) * screen_offset_per_px_y;
        ray_t eye_ray = { pixel_pos, normalized(pixel_pos - s.observer) };
        px_color += cast_ray(eye_ray, s, background_color,
          CAST_TO_OBJECT, 1.f, 0u);
      }
      img.px(x, y) = px_color / s.sample_count;
    }
    if (thread_id == 0 && display_progress) {
      float current_progress = 100.f * float(y) / s.res.y;
      float previous_progress = 100.f * (float(y) - 1) / s.res.y;
      if (std::floor(current_progress) != std::floor(previous_progress)) {
        std::cout << current_progress << "%" << std::endl;
      }
    }
  }
}

image generate_image(const scene_t& s, unsigned thread_count,
  bool display_progress)
{
  const vec3f screen_offset_per_px_x = s.screen_offset_per_px_x();
  const vec3f screen_offset_per_px_y = s.screen_offset_per_px_y();

  image img(s.res.x, s.res.y);
  std::vector<std::thread> threads(thread_count);

  for (unsigned thread_id = 0u; thread_id < threads.size(); ++thread_id) {
    threads[thread_id] = std::thread(std::bind(generate_pixels, thread_id,
      thread_count,
      std::cref(s),
      std::cref(screen_offset_per_px_x),
      std::cref(screen_offset_per_px_y),
      display_progress,
      std::ref(img)));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  return img;
}

int main(int argc, char** argv) {
  user_inputs user = parse_inputs(argc, argv);
  if (user.requests_help) {
    std::cout << help_text << std::endl;
    std::exit(EXIT_OK);
  } else if (user.requests_help_scene) {
    std::cout << scene_file_help_text << std::endl;
    std::exit(EXIT_OK);
  }

  const scene_t scene = try_load_scene_from_file(
    get_with_default(user.scene_file, "world.yml"), EXIT_FAIL_LOAD);
  create_photon_map(scene);

  image img = generate_image(scene, user.thread_count, user.display_progress);
  img.clamp_colors();
  if (!img.save_as_png(get_with_default(user.output_file, "output.png"))) {
    return EXIT_FAIL_SAVE;
  }
  
  return EXIT_OK;
}
