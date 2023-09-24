#include <iostream>
#include <string>
#include "diy/neo_ecs.hpp"

template<class Ty, int idx>
struct TypeSlot {
  constexpr static int value = idx;
};

int GenerateTypeID() {
  static int value = 0;
  return value++;
}

template<class Ty>
const int type_value = GenerateTypeID();

template<class... Ty>
void Types2Slots() {
  (std::cout  << ... << type_value<Ty>);
}

void test_neo_ecs() {
  using A = float;
  using B = int;
  sim::neo::ecs::World world;
  auto& ent = world.entity();

  ent.set(A{1}, B{2});
  auto [d, c, b, a] = ent.get<A,B,B,A>();

  std::cout << d << "," << c << "," << b << "," << a << "\n";
}

int main() {
  Types2Slots<int, float, std::string, int, float>();
}