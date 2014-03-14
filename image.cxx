#include <limits>
#include <stdexcept>
#include <sstream>
#include "image.h"

image::image(unsigned width, unsigned height)
  : pixels(width * height)
  , width_(width)
{
  size_t w = width;
  size_t h = height;
  size_t overflow_point = std::numeric_limits<unsigned>::max();
  if (w*h > overflow_point) {
    std::stringstream buf;
    buf << "Image too large. Dimensions of [" << width << ","
      << height << "] require " << w*h << "pixels.";
    throw std::invalid_argument(buf.str());
  }
}
