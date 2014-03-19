#ifndef VEC4F_H
#define VEC4F_H

#include <array>

struct vec4f : public std::array<float, 4> {
  vec4f();
  vec4f(float x, float y, float z, float w);

  float x() const;
  float y() const;
  float z() const;
  float w() const;
};

inline vec4f::vec4f(){
}

inline vec4f::vec4f(float x, float y, float z, float w) {
  (*this)[0] = x;
  (*this)[1] = y;
  (*this)[2] = z;
  (*this)[3] = w;
}

inline float vec4f::x() const {
  return (*this)[0];
}

inline float vec4f::y() const {
  return (*this)[1];
}

inline float vec4f::z() const {
  return (*this)[2];
}

inline float vec4f::w() const {
  return (*this)[3];
}

#endif
