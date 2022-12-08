#include <iostream>
#include "ecs/ecs.h"

int main() {
    ecs::World world;
    auto entity = world.entity()
    .add<int>()
    .set<float>()
    .set<int>(5).add<float>();

    printf("value: %d\n", entity.get<int>());
}
