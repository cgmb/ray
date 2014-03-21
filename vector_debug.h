#ifndef VECTOR_DEBUG_H
#define VECTOR_DEBUG_H

#include <iostream>
#include "vec3f.h"

std::ostream& operator<<(std::ostream& out, const vec3f& v) {
  return out << "[" << v[0] << ", " << v[1] << ", " << v[2] << "]";
}

#endif
