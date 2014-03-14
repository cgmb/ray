#include <iostream>
#include "test/test.h"

#define COLOR_GREEN     "\x1b[32m"
#define COLOR_RED       "\x1b[31m"
#define COLOR_RESET     "\x1b[0m"

const char* color(size_t failure_count) {
  return failure_count == 0 ? COLOR_GREEN : COLOR_RED;
}

const char* end_punctuation(size_t failure_count) {
  return failure_count == 0 ? "." : ":";
}

extern test_results test_geometry();
extern int test_image();

int main(int argc, char** argv) {
  test_results results = test_geometry();
  auto end_it = std::remove_if(results.begin(), results.end(),
    [](const test_result& x)->bool{ return x.passed; });
  size_t failure_count = std::distance(results.begin(), end_it);
  std::cout << color(failure_count) << "Completed with "
    << failure_count << " failures" << end_punctuation(failure_count)
    << COLOR_RESET << std::endl;
  for (auto it = results.begin(); it != end_it; ++it) {
    std::cout << COLOR_RED << it->message << COLOR_RESET << std::endl;
  }
  return failure_count;
}
