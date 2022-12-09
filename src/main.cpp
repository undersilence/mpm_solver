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
        entities.emplace_back(world.entity().add<Position>({1, 1}));
    }
    for(int i = 0; i < N/2; i++) {
        entities[i].add<Velocity>({-1, -1});
    }
    
    for(auto& e : entities) {
        int flags = 0;
        flags |= (int)e.has<Position>();
        flags |= (int)e.has<Velocity>() << 1;
        if (flags == 3) {
            auto& pos = e.get<Position>();
            auto& vel = e.get<Velocity>();
            e.set<Position>({pos.x + vel.x, pos.y + vel.y});
        }
        auto& pos = e.get<Position>();
        printf("e: %llu, pos: (%f, %f)\n", e.id, pos.x, pos.y);
    }
}
