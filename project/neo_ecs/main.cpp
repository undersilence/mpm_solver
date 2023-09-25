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

  ent.add(A{1.1234}, B{233}, std::string{"str"}, double{4.1231});
  auto [d, c, x, y, b, a] = ent.get<A,B,std::string, double, B,A>();
  std::cout << d << "," << c << "," << b << "," << a << "," << x << "," << y << "\n";

  ent.del<A, B>();
  auto [xx, yy] = ent.get<std::string, double>();
  std::cout << xx << "," << yy << "\n";  

  xx += "_modified";
  yy *= 3.1415926;

  ent.add<A, B>(3.1234f, 332);
  auto [a0, b0, c0, d0, e0, f0] = ent.get<B, A, std::string, double, A, B>();

  ent.set<A, B>(3.12345f, 3332);
  std::cout << a0 << "," << b0 << "," << c0 << "," << d0 << "," << e0 << "," << f0 << "\n";

  auto Q = world.query<A, B, A, B, double, std::string>();
  Q.for_each([]<class... Ty>(Ty... args) {
    ((std::cout << args << ","), ...);
  });
}

int main() {
  // Types2Slots<int, float, std::string, int, float>();
  test_neo_ecs();
}