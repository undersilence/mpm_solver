#include <iostream>
#include "diy/neo_ecs.hpp"


int main() {

  using A = float;
  using B = int;
  sim::neo::ecs::World world;
  auto& ent = world.entity();

  ent.set(A{1}, B{2});
  auto [d, c, b, a] = ent.get<A,B,B,A>();

  std::cout << d << "," << c << "," << b << "," << a << "\n";
}