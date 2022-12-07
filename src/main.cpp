#include <iostream>
#include "ecs/ecs.h"

int main() {
    ecs::World world;

    world.component<float>();
    world.component<int>();
}
