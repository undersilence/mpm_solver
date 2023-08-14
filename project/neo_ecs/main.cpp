#include <iostream>
#include "diy/neo_ecs.hpp"


int main() {

  using A = float;
  using B = int;
  using C = A;
  using D = B;

  sim::neo::ecs::World world;
  auto& ent = world.entity();

  ent.set(A{1}, B{2}, C{3}, D{4});
  auto [d, c, b, a] = ent.get<D,C,B,A>();

  std::cout << d << "," << c << "," << b << "," << a << "\n";
}