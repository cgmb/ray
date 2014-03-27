#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <png.h>
#include "image.h"

namespace {

typedef std::unique_ptr<FILE,int(*)(FILE*)> unique_file_ptr;

int null_friendly_fclose(FILE* file) {
  if (!file) {
    return 0;
  }
  fclose(file);
  return 0;
}

struct color_24bit {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct image_24bit {
  image_24bit(size_t width, size_t height)
    : pixels(width * height)
    , width(width)
    , height(height)
  {
  }

  std::vector<color_24bit> pixels;
  size_t width;
  size_t height;
};

struct png_write_context {
  png_write_context()
  : png_ptr(0)
  , info_ptr(0)
  {
  }

  ~png_write_context() {
    png_destroy_write_struct(&png_ptr, &info_ptr);
  }

  png_structp png_ptr;
  png_infop info_ptr;
};

/* Saving 24bit PNG based on tutorial from http://www.lemoda.net/c/write-png/

   todo: save the data directly from the img buffer
   http://stackoverflow.com/q/1821806/331041
*/
bool save_png_to_file(const image_24bit& bitmap, const char* path)
{
  unique_file_ptr file(fopen(path, "wb"), null_friendly_fclose);
  if (!file) {
    return false;
  }

  png_write_context ctx;
  ctx.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!ctx.png_ptr) {
    return false;
  }

  ctx.info_ptr = png_create_info_struct(ctx.png_ptr);
  if (!ctx.info_ptr) {
    return false;
  }

  // error handling
  if (setjmp(png_jmpbuf(ctx.png_ptr))) {
    return false;
  }

  // image attributes
  const int depth = 8;
  png_set_IHDR(ctx.png_ptr, ctx.info_ptr, bitmap.width, bitmap.height, depth,
    PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);

  // init rows
  png_byte** row_pointers = (png_byte**)png_malloc(ctx.png_ptr,
    bitmap.height * sizeof(png_byte*));
  const int pixel_size = 3; // not sure what this is for
  auto pixel_it = bitmap.pixels.begin();
  for (size_t y = 0; y < bitmap.height; ++y) {
    png_byte* row = (png_byte*)png_malloc(ctx.png_ptr,
      sizeof(uint8_t) * bitmap.width * pixel_size);
    row_pointers[y] = row;
    for (size_t x = 0; x < bitmap.width; ++x) {
      const color_24bit pixel = *pixel_it++;
      *row++ = pixel.red;
      *row++ = pixel.green;
      *row++ = pixel.blue;
    }
  }

  // write out
  png_init_io(ctx.png_ptr, file.get());
  png_set_rows(ctx.png_ptr, ctx.info_ptr, row_pointers);
  png_write_png(ctx.png_ptr, ctx.info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  for (size_t y = 0; y < bitmap.height; ++y) {
    png_free(ctx.png_ptr, row_pointers[y]);
  }
  png_free(ctx.png_ptr, row_pointers);

  return true;
}

uint8_t f2p(float f) {
  return f * 255;
}

color_24bit vec3f_to_24bit_color(const vec3f& v) {
  return color_24bit{ f2p(v[0]), f2p(v[1]), f2p(v[2]) };
}

vec3f clamp_color(vec3f color) {
  return vec3f(std::min(color[0], 1.f),
               std::min(color[1], 1.f),
               std::min(color[2], 1.f));
}
  
} // namespace

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

unsigned image::height() const {
  return pixels.size() / width_;
}

unsigned image::width() const {
  return width_;
}

bool image::save_as_png(const char* path) const {
  image_24bit output_image(width(), height());
  std::transform(pixels.begin(), pixels.end(),
    output_image.pixels.begin(), vec3f_to_24bit_color);
  return save_png_to_file(output_image, path);
}

void image::clamp_colors() {
  std::transform(pixels.begin(), pixels.end(),
    pixels.begin(), clamp_color);
}
