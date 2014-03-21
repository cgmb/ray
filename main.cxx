#include <iostream>
#include <vector>
#include "geometry.h"
#include "image.h"
#include "scene.h"
#include "vec3f.h"

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

int main(int argc, char** argv) {
  user_inputs user = parse_inputs(argc, argv);
  if (user.requests_help) {
    print_help();
    std::exit(EXIT_OK);
  }

  scene s = try_load_scene_from_file(
    get_with_default(user.scene_file, "scene.yml"), EXIT_FAIL_LOAD);

  const vec3f observer = s.observer;
  const vec3f screen_top_left = s.screen_top_left;
  const std::vector<sphere> geometry = s.spheres;
  const std::vector<vec3f> sphere_colors = s.sphere_colors;
  const std::vector<light> lights = s.lights;
  const resolution res = s.res;

  const vec3f screen_offset_per_px_x = s.screen_offset_per_px_x();
  const vec3f screen_offset_per_px_y = s.screen_offset_per_px_y();

  image img(res.x, res.y);

  for (unsigned y = 0u; y < res.y; ++y) {
    for (unsigned x = 0u; x < res.x; ++x) {
      vec3f color = { 0, 0, 0 };
      vec3f pixel_pos = screen_top_left +
        x * screen_offset_per_px_x +
        y * screen_offset_per_px_y;
      ray eye_ray = { pixel_pos, normalized(pixel_pos - observer) };
      ray_sphere_intersect rsi = cast_ray(eye_ray, geometry);
      if (rsi.intersect_exists(geometry)) {
//        vec3f pos = eye_ray.position_at(rsi.t);
        color = sphere_colors[rsi.index_in(geometry)];
      }

      img.px(x, y) = color;
    }
  }

  if (!img.save_as_png(get_with_default(user.output_file, "output.png"))) {
    return EXIT_FAIL_SAVE;
  }
  
  return EXIT_OK;
}
