#include <iostream>
#include <string>
#include "diy/neo_ecs.hpp"

template<class Ty, int idx>
struct TypeSlot {
  constexpr static int value = idx;
};

static int value = 0;
template<class Ty>
const int type_value = value++;

template<class... Ty>
void Types2Slots() {
  (std::cout  << ... << type_value<Ty>);
}

void test_neo_ecs() {
  using A = float;
  using B = int;
  sim::neo::ecs::World world;
  auto& ent = world.entity();

  ent.set(A{1}, B{2}, std::string{"123"}, double{4.1231});
  auto [d, c, x, y, b, a] = ent.get<A,B,std::string, double, B,A>();

  std::cout << d << "," << c << "," << b << "," << a << "," << x << "," << y << "\n";
}

int main() {
  Types2Slots<int, float, std::string, int, float>();
  test_neo_ecs();
}