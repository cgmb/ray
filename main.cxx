#include <algorithm>
#include <functional>
#include <vector>
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

int main(int argc, char** argv) {
  vec3f observer = { 0, 0, -10 };
  vec3f screen_top_left = { 0, 0, 0 };
  vec3f screen_top_right = { 0, 0, 0 };
  vec3f screen_bottom_right = { 0, 0, 0 };
  resolution res = { 100, 100 };

  vec3f screen_offset_x = screen_top_right - screen_top_left;
  vec3f screen_offset_y = screen_bottom_right - screen_top_right;

  vec3f screen_offset_per_px_x = (1.f / res.x) * screen_offset_x;
  vec3f screen_offset_per_px_y = (1.f / res.y) * screen_offset_y;

  image img(res.x, res.y);

  std::vector<sphere> geometry;
  sphere s = { vec3f(0, 0, 10), 3 };
  geometry.push_back(s);

  std::vector<light> lights;
  light l = { vec3f(0, 0, -10), vec3f(1, 1, 1) };
  lights.push_back(l);

  for (unsigned y = 0u; y < res.y; ++y) {
    for (unsigned x = 0u; x < res.x; ++x) {
      vec3f color = { 0, 0, 0 };
      vec3f pixel_pos = screen_top_left +
        x * screen_offset_per_px_x +
        y * screen_offset_per_px_y;
      ray eye_ray = { observer, normalized(pixel_pos - observer) };
      auto intersects_eye_ray_at = std::bind(near_intersect_param, eye_ray, _1);
      std::vector<float> intersections(geometry.size());
      std::transform(geometry.begin(), geometry.end(), 
        intersections.begin(), intersects_eye_ray_at);
      auto near_it = std::min_element(intersections.begin(), intersections.end());
      auto near_geometry_it = geometry.begin() +
        std::distance(intersections.begin(), near_it);
      if (near_it != intersections.end() && !std::isnan(*near_it)) {
        // there was an intersection
        vec3f pos = eye_ray.position_at(*near_it);
        (void)pos; (void)near_geometry_it;
        color = vec3f(1, 0, 1);
      }

      img.px(x, y) = color;
    }
  }

  if (!img.save_as_png("output.png")) {
    return 1;
  }
  
  return 0;
}
