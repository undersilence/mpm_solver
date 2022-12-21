#include <cstdio>
#include <vector>
#include <any>
#include "ecs/vec.hpp"
#include "utils/timer.hpp"

static int move_count = 0;
static int copy_count = 0;
static int delete_count = 0;

class Dummy {
public:
  int x;
  Dummy() = default;
  Dummy(Dummy&&) = default;
  Dummy(int x) : x(x) {}
  Dummy(const Dummy& d) : x(d.x) {
    copy_count++;
  }
  Dummy& operator=(const Dummy&) = default;
  ~Dummy() {
    delete_count++;
  }
};

static int N = 1000000;
static int STEPS = 100;

void test_std_vector(const std::vector<Dummy>& test) {
  move_count = copy_count = delete_count = 0;
  {
    FUNCTION_TIMER();
    auto vec = test;
    int ans = 0;
    {
      SCOPED_TIMER("std::vector::simulation");
      for (size_t t = 0; t < STEPS; ++t) {
        for (size_t i = 0; i < vec.size(); i++) {
          vec[i].x = vec[i].x * vec[i].x * 0.5f;
        }
      }
    }
    for (size_t i = 0; i < vec.size(); i++) {
      ans += vec[i].x % 9999;
    }
    LOG_INFO("sum from std::vector {}", ans);
  }
  LOG_INFO("move: {}, copy: {}, delete: {}", move_count, copy_count, delete_count);
}

void test_any(const std::vector<Dummy>& test) {
  move_count = copy_count = delete_count = 0;
  {
    FUNCTION_TIMER();
    std::vector<std::any> vec_any;
    vec_any.reserve(N);
    for (int i = 0; i < N; i++) {
      vec_any.emplace_back(test[i]);
    }
    int ans = 0;
    {
      SCOPED_TIMER("any::simulation");
      for (size_t t = 0; t < STEPS; ++t) {
        for (size_t i = 0; i < vec_any.size(); i++) {
          auto& d = std::any_cast<Dummy&>(vec_any[i]);
          d.x = d.x * d.x * 0.5f;
        }
      }
    }
    for (size_t i = 0; i < vec_any.size(); i++) {
      ans += std::any_cast<Dummy&>(vec_any[i]).x % 9999;
    }
    LOG_INFO("sum from std::vector<std::any> {}", ans);
  }
  LOG_INFO("move: {}, copy: {}, delete: {}", move_count, copy_count, delete_count);
}

void test_vec_core_t(const std::vector<Dummy>& test) {
  move_count = copy_count = delete_count = 0;
  {
    FUNCTION_TIMER();
    auto vec = sim::ecs::create_vec<Dummy>();
    vec.insert(vec.end(), test.data(), test.data() + test.size());
    int idx = 0;
    int ans = 0;
    {
      SCOPED_TIMER("vec_core_t::simulation");
      for (size_t t = 0; t < STEPS; ++t) {
        auto p = (Dummy*)vec.begin(); // extract ptr first get best performance
        for (int i = 0; i < N; i++) {
          p[i].x = p[i].x * p[i].x * 0.5f;
        }
      }
    }
    auto p = (Dummy*)vec.begin();
    for (int i = 0; i < N; i++) {
      ans += p[i].x % 9999;
    }
    LOG_INFO("sum from vec_core_t {}", ans);
  }
  LOG_INFO("move: {}, copy: {}, delete: {}", move_count, copy_count, delete_count);
}

int main() {
  std::vector<Dummy> test;
  for(int i = 0; i < N; i++) {
    test.emplace_back(i * i);
  }

  test_std_vector(test);
  // test_any(test);
  test_vec_core_t(test);
  return 0;
}