#ifndef VEC3F_H
#define VEC3F_H

// My addition to the md2.h reader.
// vec3_t was originally float[3], but C arrays behave oddly.
// Switching to std::array makes it behave more like a normal type.

#include <array>
#include "vector_math.h"

struct vec3f : public std::array<float, 3> {
  vec3f();
  vec3f(float x, float y, float z);

  float x() const;
  float y() const;
  float z() const;

  operator float*();
  operator const float*() const;

  vec3f& operator+=(const vec3f& rhs);
  vec3f& operator-=(const vec3f& rhs);
};

inline vec3f::vec3f(){
}

inline vec3f::vec3f(float x, float y, float z) {
  (*this)[0] = x;
  (*this)[1] = y;
  (*this)[2] = z;
}

inline float vec3f::x() const {
  return (*this)[0];
}

inline float vec3f::y() const {
  return (*this)[1];
}

inline float vec3f::z() const {
  return (*this)[2];
}

inline vec3f::operator float*() {
  return this->data();
}

inline vec3f::operator const float*() const {
  return this->data();
}

inline vec3f& vec3f::operator+=(const vec3f& rhs) {
  vec3f& lhs = *this;
  lhs[0] += rhs[0];
  lhs[1] += rhs[1];
  lhs[2] += rhs[2];
  return lhs;
}

inline vec3f& vec3f::operator-=(const vec3f& rhs) {
  vec3f& lhs = *this;
  lhs[0] -= rhs[0];
  lhs[1] -= rhs[1];
  lhs[2] -= rhs[2];
  return lhs;
}

inline vec3f operator+(vec3f lhs, const vec3f& rhs) {
  lhs += rhs;
  return lhs;
}

inline vec3f operator-(vec3f lhs, const vec3f& rhs) {
  lhs -= rhs;
  return lhs;
}

inline vec3f operator/(const vec3f& lhs, float rhs) {
  return vec3f(lhs[0] / rhs, lhs[1] / rhs, lhs[2] / rhs);
}

inline vec3f operator*(float lhs, const vec3f& rhs) {
  return vec3f(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2]);
}

inline vec3f operator*(const vec3f& lhs, float rhs) {
  return rhs * lhs;
}

inline vec3f normalized(vec3f x) {
  normalize_v3f(x);
  return x;
}

inline float magnitude(const vec3f& x) {
  return magnitude_v3f(x);
}

inline float dot(const vec3f& x, const vec3f& y) {
  return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];
}

#endif
