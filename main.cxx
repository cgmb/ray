#include "image.h"
#include "vec3f.h"
#include "vector_math.h"
#include "vector_debug.h"

struct resolution {
  unsigned x;
  unsigned y;
};

int main(int argc, char** argv) {
  vec3f observer = { 0, 0, 0 };
  vec3f screen_top_left = { 0, 0, 0 };
  vec3f screen_top_right = { 0, 0, 0 };
  vec3f screen_bottom_right = { 0, 0, 0 };
  resolution res = { 100, 100 };

  vec3f screen_offset_x = screen_top_right - screen_top_left;
  vec3f screen_offset_y = screen_bottom_right - screen_top_right;

  vec3f screen_offset_x_per_px = (1.f / res.x) * screen_offset_x;
  vec3f screen_offset_y_per_px = (1.f / res.y) * screen_offset_y;

  image i(res.x, res.y);

  for (unsigned y = 0u; y < res.y; ++y) {
    for (unsigned x = 0u; x < res.x; ++x) {
      vec3f px = x * screen_offset_x_per_px;
      vec3f py = y * screen_offset_y_per_px;
      vec3f p = screen_top_left + px + py;
      vec3f d = p - observer;
      (void)d;
    }
  }
  
  return 0;
}

