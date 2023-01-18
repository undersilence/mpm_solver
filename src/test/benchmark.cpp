#include "diy/ecs.hpp"
#include "utils/timer.hpp"
#include <iostream>
#include <vector>

#include "ext/flecs.h"

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

struct Particle {
  Position p;
  Velocity v;
  Force f;
  Mass m;
  Particle() = default;
  Particle(Particle&&) = default;
};

static constexpr size_t N = 1000000;
static constexpr size_t STEPS = 10;

double test_AOS() {
  FUNCTION_TIMER();
  double total_p{};
  std::vector<Particle> particles;
  {
    SCOPED_TIMER("AOS::init");
    for (size_t i = 0; i < N; ++i) {
      Particle particle{};
      particle.m = {1.0f};
      particle.f = {1.0f, 1.0f, 1.0f};
      particle.v = {0.0f, 0.0f, 0.0f};
      particle.p = {0.0f, 0.0f, 0.0f};
      particles.emplace_back(std::move(particle));
    }
  }

  {
    SCOPED_TIMER("AOS::simulation");
    for (size_t t = 0; t < STEPS; ++t) {
      for (size_t i = 0; i < N; ++i) {
        auto& part = particles[i];
        part.f.y *= 0.95f;
        part.v.x += part.f.x * part.m.value;
        part.v.y += part.f.y * part.m.value;
        part.v.z += part.f.z * part.m.value;
        part.p.x += part.v.x;
        part.p.y += part.v.y;
        part.p.z += part.v.z;
      }
    }
  }
  {
    SCOPED_TIMER("AOS::summary");
    for (size_t i = 0; i < N; ++i) {
      total_p += particles[i].p.x + particles[i].p.y + particles[i].p.z;
    }
  }
  return total_p;
}

double test_SOA() {
  FUNCTION_TIMER();
  std::vector<Position> p;
  std::vector<Velocity> v;
  std::vector<Force> f;
  std::vector<Mass> m;
  double total_p = 0.0f;
  {
    SCOPED_TIMER("SOA::init");
    p = std::vector<Position>(N, {0});
    v = std::vector<Velocity>(N, {0});
    f = std::vector<Force>(N, {1, 1, 1});
    m = std::vector<Mass>(N, {1});
  }
  {
    SCOPED_TIMER("SOA::simulation");
    for(size_t t = 0; t < STEPS; ++t) {
      for(size_t i = 0; i < N; ++i) {
        f[i].y *= 0.95f;
        v[i].x += f[i].x * m[i].value;
        v[i].y += f[i].y * m[i].value;
        v[i].z += f[i].z * m[i].value;
        p[i].x += v[i].x;
        p[i].y += v[i].y;
        p[i].z += v[i].z;
      }
    }
  }
  {
    SCOPED_TIMER("SOA::summary");
    for(size_t i = 0; i < N; ++i) {
      total_p += p[i].x + p[i].y + p[i].z;
    }
  }
  return total_p;
}

double test_ECS() {
  FUNCTION_TIMER();
  using namespace sim;
  ecs::World world;
  std::vector<ecs::Entity> particles;
  double total_p{};
  {
    SCOPED_TIMER("ECS::init");
    for(size_t i = 0; i < N; ++i) {
      particles.emplace_back(world.entity().add<Position, Velocity, Force, Mass>(
          {0}, {0}, {1.0f, 1.0f, 1.0f}, {1.0f}
          ));
    }
  }
  {
    SCOPED_TIMER("ECS::simulation");
    auto Q = world.query<Position, Velocity, Force, Mass>();
    for(size_t t = 0; t < STEPS; ++t) {
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

double test_FLECS() {
  FUNCTION_TIMER();
  flecs::world world;
  auto Q = world.query<Position, Velocity, Force, Mass>();
  double total_p{};
  {
    SCOPED_TIMER("FLECS::init");
    for (size_t i = 0; i < N; i++) {
      auto e = world.entity();
      e.set<Position>({}).set<Velocity>({}).set<Force>({1, 1, 1}).set<Mass>({1});
    }
  }
  {
    SCOPED_TIMER("FLECS::simulation");
    for(size_t t = 0; t < STEPS; ++t) {
      Q.each([](Position& p, Velocity& v, Force& f, Mass& m) {
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
    SCOPED_TIMER("FLECS::summary");
    auto QQ = world.query<Position>();
    QQ.each([&](Position& p) {
      total_p += p.x + p.y + p.z;
    });
  }
  return total_p;
}

int main() {
  LOG_INFO("Start AOS/SOA/ECS benchmark...");

  LOG_INFO("total_p is {} from test_AOS", test_AOS());
  LOG_INFO("total_p is {} from test_SOA", test_SOA());
  LOG_INFO("total_p is {} from test_ECS", test_ECS());
  LOG_INFO("total_p is {} from test_FLECS", test_FLECS());

  return 0;
}