#include "diy/ecs.hpp"
#include <iostream>
#include <vector>

struct Position {
  float x, y;
  Position() = default;
  Position(Position&&) = default;
  Position(const Position& other) {
    x = other.x;
    y = other.y;
    static int copy_count = 0;
    printf("Copy components position (%f, %f), %d\n", x, y, ++copy_count);
  }
  Position(float x, float y) : x(x), y(y){}
};

struct Velocity {
  float x, y;
};

int main() {
  sim::ecs::World world;
  int N = 10;
  std::vector<sim::ecs::Entity> entities;
  for (int i = 0; i < N; i++) {
    entities.emplace_back(world.entity().set<Position>());
  }

  for(int i = 0; i < N; i++) {
    if (i % 2) {
      entities[i].set<Position, Velocity>({(float)i, (float)i}, {(float)i * i, (float)i * i});
    }
  }

  // std::for_each support
  auto Q = world.query<Position, Velocity>();
  std::for_each(std::begin(Q), std::end(Q), [](std::tuple<Position&, Velocity&> pack) {
    // auto& p = std::get<0>(pack);
    auto& [p, v] = pack;
    printf("position (%f, %f), velocity (%f, %f)\n", p.x, p.y, v.x, v.y);
    v.x *= 0.99f;
    v.y *= 0.99f;
    p.x += v.x;
    p.y += v.y;
  });

  std::for_each(std::begin(Q), std::end(Q), [](std::tuple<Position&, Velocity&> pack) {
    // auto& p = std::get<0>(pack);
    auto& [p, v] = pack;
    printf("updated position (%f, %f), updated velocity (%f, %f)\n", p.x, p.y, v.x, v.y);
  });

  //    for(auto& e : entities) {
  //        int flags = 0;
  //        flags |= (int)e.has<Position>();
  //        flags |= (int)e.has<Velocity>() << 1;
  //        if (flags == 3) {
  //            auto& pos = e.get<Position>();
  //            auto& vel = e.get<Velocity>();
  //            e.set<Position>({pos.x + vel.x, pos.y + vel.y});
  //        }
  //        auto& pos = e.get<Position>();
  //        printf("e: %llu, pos: (%f, %f)\n", e.id, pos.x, pos.y);
  //    }
}
