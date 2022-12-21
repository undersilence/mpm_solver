#include "ecs/vec.hpp"
#include <cstdio>
#include <vector>

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

int main() {
  std::vector<Dummy> test;
  for(int i = 0; i < 10; i++) {
    test.emplace_back(i * i);
  }
  {
    move_count = copy_count = delete_count = 0;
    auto vec = sim::ecs::create_vec<Dummy>();
    vec.insert(vec.end(), test.data(), test.data() + test.size());
    int idx = 0;
    for (auto iter = vec.begin(); iter != vec.end(); vec.advance(iter, 1)) {
      Dummy& d = *(Dummy*)iter;
      printf("dummy[%d] = %d\n", idx++, d.x);
    }
  }
  printf("move: %d, copy: %d, delete: %d\n", move_count, copy_count, delete_count);
  return 0;
}