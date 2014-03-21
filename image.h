#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include "vec3f.h"

struct image {
  image(unsigned width, unsigned height);

  std::vector<vec3f> pixels;

  vec3f& px(unsigned x, unsigned y);
  const vec3f& px(unsigned x, unsigned y) const;

  bool save_as_png(const char* path) const;
  void clamp_colors();

private:
  unsigned height() const;
  unsigned width() const;

private:
  unsigned width_;
};

inline vec3f& image::px(unsigned x, unsigned y) {
  return pixels[y * width_ + x];
}

inline const vec3f& image::px(unsigned x, unsigned y) const {
  return pixels[y * width_ + x];
}

#endif
