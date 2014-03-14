#ifndef TEST_H
#define TEST_H

#include <string>
#include <vector>
#include <algorithm>

struct test_result {
  bool passed;
  std::string message;
};

struct test_results : public std::vector<test_result> {
  int failure_count() const {
    return std::count_if(begin(), end(),
      [](const test_result& x)->bool{ return !x.passed; });
   }
};

inline test_results operator%(const test_result& lhs, const test_result& rhs) {
  test_results results;
  results.push_back(lhs);
  results.push_back(rhs);
  return results;
}

inline test_results& operator%(test_results& lhs, const test_result& rhs) {
  lhs.push_back(rhs);
  return lhs;
}

inline test_results& operator%(const test_result& lhs, test_results& rhs) {
  return rhs % lhs;
}

#define RTEST(name, test) test_result name() { return { test, #name }; }

#endif
