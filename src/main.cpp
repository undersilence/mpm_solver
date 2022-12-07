#include <iostream>
#include "ecs/ecs.h"

int main() {
    ecs::World world;
    ecs::Type empty_type;
    printf("component id: %llu\n", world.component<float>());
    printf("component id: %llu\n", world.component<int>());
}
