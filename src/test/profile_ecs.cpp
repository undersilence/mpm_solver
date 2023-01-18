#include "diy/ecs.hpp"
#include "utils/timer.hpp"
#include <iostream>
#include <vector>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

struct Force {
  float x, y, z;
};

struct Mass {
  float value;
};

static constexpr size_t N = 1000000;
static constexpr size_t STEPS = 10;

double test_ECS() {
  FUNCTION_TIMER();
  using namespace sim;
  ecs::World world;
  std::vector<ecs::Entity> particles;
  double total_p{};
  {
    SCOPED_TIMER("ECS::init");
    for (size_t i = 0; i < N; ++i) {
      particles.emplace_back(world.entity().add<Position, Velocity, Force, Mass>(
          {0}, {0}, {1.0f, 1.0f, 1.0f}, {1.0f}));
    }
  }
  {
    SCOPED_TIMER("ECS::simulation");
    auto Q = world.query<Position, Velocity, Force, Mass>();
    for (size_t t = 0; t < STEPS; ++t) {
      std::for_each(std::begin(Q), std::end(Q), [](auto data) {
        auto& [p, v, f, m] = data;
        f.y *= 0.95f;
        v.x += f.x * m.value;
        v.y += f.y * m.value;
        v.z += f.z * m.value;
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
      });
    }
  }
  {
    SCOPED_TIMER("ECS::summary");
    auto Q = world.query<Position>();
    std::for_each(std::begin(Q), std::end(Q), [&](auto data) {
      auto& [p] = data;
      total_p += p.x + p.y + p.z;
    });
  }
  return total_p;
}

int main() {
  test_ECS();
  return 0;
}