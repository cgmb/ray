
#include <vector>
#include <iostream>
#include <cmath>
#include "vec3f.h"
#include "vector_debug.h"

int main() {
  unsigned e = 50;
  float r = 5;
  std::vector<vec3f> pts(e+1*2);
  float h = 10;
  float y1 = -5;
  float z_offset = 10;
  std::cout << "    - vertexes:" << std::endl;
  for (unsigned i = 0; i <= e; ++i) {
    float x = -r*cos(M_PI*float(i)/e);
    float z = r*sin(M_PI*float(i)/e-M_PI) + z_offset;
    float y = y1;
    std::cout << "      - [" << y << ", " << x << ", " << z << "]" << std::endl;
    y = y1 + h;
    std::cout << "      - [" << y << ", " << x << ", " << z << "]" << std::endl;
  }
  std::cout << "      indexes: [";
  for (unsigned i = 0; i <= e-1; ++i) {
    std::cout << 2*i << ", " << 2*(i+1)+1 << ", " << 2*i+1 << ", ";
    std::cout << 2*i << ", " << 2*(i+1) << ", " << 2*(i+1)+1 << ", ";
  }
  std::cout << "]" << std::endl;
}
/*
    - vertexes:
      - [-5, 5, 5]
      - [-5, -5, 5]
      - [-5, 5, 15]
      - [-5, -5, 15]
      - [5, 5, 15]
      - [5, -5, 15]
      - [5, 5, 5]
      - [5, -5, 5]
      indexes: [0, 1, 3, 3, 2, 0, 2, 3, 4, 3, 5, 4, 4, 5, 7, 7, 6, 4]
*/
