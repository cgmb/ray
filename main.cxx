#include <iostream>
#include <vector>
#include "geometry.h"
#include "image.h"
#include "scene.h"
#include "vec3f.h"

#include "vector_debug.h"

enum {
  EXIT_OK = 0,
  EXIT_FAIL_SAVE,
  EXIT_FAIL_LOAD,
};

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
  // todo
}

const char* get_with_default(const char* primary, const char* fallback) {
  return primary ? primary : fallback;
}

enum cast_policy_t {
  CAST_TO_OBJECT,
  CAST_TO_LIGHT,
};

vec3f cast_ray(const ray_t& ray,
  const scene_t& s,
  vec3f default_color,
  cast_policy_t cast_policy,
  unsigned recursion_depth)
{
  vec3f color = default_color;

  ray_sphere_intersect rsi = cast_ray(ray, s.geometry.spheres);
  if (rsi.intersect_exists(s.geometry.spheres)) {
    if (cast_policy == CAST_TO_OBJECT) {
      vec3f pos = ray.position_at(0.9999f * rsi.t);
      material_t material = s.sphere_materials[rsi.index_in(s.geometry.spheres)];
      float solid_component = material.opacity - material.reflectivity;

      if (solid_component > 0.f) {
        vec3f light_color(0,0,0);
        for (const light_t& light : s.lights) {
          ray_t light_ray = { pos, normalized(light.position - pos) };
          light_color += cast_ray(light_ray, s, light.color,
            CAST_TO_LIGHT, recursion_depth + 1u);
        }
        color += solid_component * material.color * light_color;
      }

      if (material.reflectivity > 0.f) {
        vec3f normal = rsi.near_geometry_it->normal_at(pos);
        ray_t reflected_ray = { pos, reflected(ray.direction, normal) };
        if (recursion_depth < 10u) {
          color += material.reflectivity * cast_ray(reflected_ray, s,
            default_color, cast_policy, recursion_depth + 1u);
        }
      }
    } else { // cast_policy == CAST_TO_LIGHT
      color = vec3f(0,0,0);
    }
  }
  return color;
}

image generate_image(const scene_t& s) {
  const vec3f observer = s.observer;
  const resolution_t res = s.res;
  const vec3f screen_top_left = s.screen_top_left;
  const vec3f screen_offset_per_px_x = s.screen_offset_per_px_x();
  const vec3f screen_offset_per_px_y = s.screen_offset_per_px_y();

  image img(res.x, res.y);

  for (unsigned y = 0u; y < res.y; ++y) {
    for (unsigned x = 0u; x < res.x; ++x) {
      vec3f background_color = { 0, 0, 0 };
      vec3f pixel_pos = screen_top_left +
        x * screen_offset_per_px_x +
        y * screen_offset_per_px_y;
      ray_t eye_ray = { pixel_pos, normalized(pixel_pos - observer) };
      img.px(x, y) = cast_ray(eye_ray, s, background_color, CAST_TO_OBJECT, 0u);
    }
  }

  return img;
}

int main(int argc, char** argv) {
  user_inputs user = parse_inputs(argc, argv);
  if (user.requests_help) {
    print_help();
    std::exit(EXIT_OK);
  }

  const scene_t scene = try_load_scene_from_file(
    get_with_default(user.scene_file, "world.yml"), EXIT_FAIL_LOAD);

  image img = generate_image(scene);
  img.clamp_colors();
  if (!img.save_as_png(get_with_default(user.output_file, "output.png"))) {
    return EXIT_FAIL_SAVE;
  }
  
  return EXIT_OK;
}
