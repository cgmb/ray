#ifndef TEXTURE_H
#define TEXTURE_H

#include <functional>
#include "vec3f.h"

typedef std::function<vec3f(const vec3f&)> tex3d_lookup_t;

/* Texturing algorithms
*/
namespace algo_texture {
  vec3f checkerboard_3d(const vec3f& position,
    const vec3f& primary_color, const vec3f& secondary_color);

  vec3f dotsnlines_3d(const vec3f& position, float period, float line_width,
    const vec3f& primary_color, const vec3f& secondary_color);
}

#endif
