#ifndef VEC2F_H
#define VEC2F_H

struct vec2f : public std::array<float, 3> {
  vec2f();
  vec2f(float x, float y);

  float x() const;
  float y() const;
};

inline vec2f::vec2f()
{
}

inline vec2f::vec2f(float x, float y) {
    (*this)[0] = x;
    (*this)[1] = y;
}

inline float vec2f::x() const {
  return (*this)[0];
}

inline float vec2f::y() const {
  return (*this)[1];
}

#endif
