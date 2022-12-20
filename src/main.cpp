#include <iostream>
#include <vector>
#include "ecs/ecs.h"

struct Position {
    float x, y;
};

struct Velocity {
    float x, y;
};

int main() {
    ecs::World world;
    int N = 10;
    std::vector<ecs::Entity> entities; 
    for(int i = 0; i < N; i++) {
        entities.emplace_back(world.entity().add<Position>({(float)i, (float)i}));
    }
    for(int i = 0; i < N/2; i++) {
        entities[i].add<Velocity>({(float)i * i, (float)i * i});
    }

    // std::for_each support
    auto Q = world.query<Position, Velocity>();
    std::for_each(std::begin(Q), std::end(Q), [](std::tuple<Position&, Velocity&> pack) {
      // auto& p = std::get<0>(pack);
      auto& [p, v] = pack;
      printf("position (%f, %f), velocity (%f, %f)\n", p.x, p.y, v.x, v.y);
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
