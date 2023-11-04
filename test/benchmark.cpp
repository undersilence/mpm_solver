#include "utils/logger.hpp"
#include "utils/timer.hpp"
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

struct Particle {
  Position p;
  Velocity v;
  Force f;
  Mass m;
  Particle() = default;
  Particle(Particle&&) = default;
};

static constexpr size_t N = 10000000;
static constexpr size_t STEPS = 100;

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
      auto& part = particles[i];
      total_p += part.p.x + part.p.y + part.p.z + part.v.x + part.v.y + part.v.z;
      part.v.x += part.f.x * part.m.value;
      part.v.y += part.f.y * part.m.value;
      part.v.z += part.f.z * part.m.value;
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
    for (size_t t = 0; t < STEPS; ++t) {
      for (size_t i = 0; i < N; ++i) {
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
    for (size_t i = 0; i < N; ++i) {
      total_p += p[i].x + p[i].y + p[i].z + v[i].x + v[i].y + v[i].z;
      v[i].x += f[i].x * m[i].value;
      v[i].y += f[i].y * m[i].value;
      v[i].z += f[i].z * m[i].value;
    }
  }
  return total_p;
}

#ifdef TEST_NEO_ECS

#include "ecs/neo_ecs.hpp"

double test_NEO_ECS() {
  FUNCTION_TIMER();
  using namespace sim;
  ecs::World world;
  std::vector<ecs::Entity> particles;
  double total_p{0};
  {
    SCOPED_TIMER("NEO_ECS::init");
    for (size_t i = 0; i < N; ++i) {
      world.entity<Position, Velocity, Force, Mass>({0}, {0}, {1.0f, 1.0f, 1.0f}, {1.0f});
    }
  }
  auto Q = world.query<Position, Velocity, Force, Mass>();
  {
    SCOPED_TIMER("NEO_ECS::simulation");
    for (size_t t = 0; t < STEPS; ++t) {
      Q.for_each( [](auto& p, auto& v, auto& f, auto& m) {
        // LOG_INFO("check addr of ECS components {:x}, {:x}, {:x}, {:x}", (size_t)&p, (size_t)&v, (size_t)&f, (size_t)&m);
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
    SCOPED_TIMER("NEO_ECS::summary");
    auto Q = world.query<Position, Velocity, Force, Mass>();
    Q.for_each( [&total_p](auto& p, auto& v, auto& f, auto& m) {
      total_p += p.x + p.y + p.z + v.x + v.y + v.z;
      v.x += f.x * m.value;
      v.y += f.y * m.value;
      v.z += f.z * m.value;
    });
  }
  return total_p;
}

#endif

#ifdef TEST_OLD_ECS

double test_ECS() {
  FUNCTION_TIMER();
  using namespace sim::neo;
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
  auto Q = world.query<Position, Velocity, Force, Mass>();
  {
    SCOPED_TIMER("ECS::simulation");
    for (size_t t = 0; t < STEPS; ++t) {
      std::for_each(std::begin(Q), std::end(Q), [](auto data) {
        auto& [p, v, f, m] = data;
        LOG_INFO("check addr of ECS components {:x}, {:x}, {:x}, {:x}", (size_t)&p, (size_t)&v, (size_t)&f, (size_t)&m);
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
    auto Q = world.query<Position, Velocity, Force, Mass>();
    std::for_each(std::begin(Q), std::end(Q), [&](auto data) {
      auto& [p, v, f, m] = data;
      total_p += p.x + p.y + p.z + v.x;
      v.x += f.x * m.value;
      v.y += f.y * m.value;
      v.z += f.z * m.value;
    });
  }
  return total_p;
}

#endif

#ifdef TEST_FLECS
#include "flecs.h"

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
    for (size_t t = 0; t < STEPS; ++t) {
      Q.each([](Position& p, Velocity& v, Force& f, Mass& m) {
        // LOG_INFO("check addr of FLECS components {:x}, {:x}, {:x}, {:x}", (size_t)&p, (size_t)&v, (size_t)&f, (size_t)&m);
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
    auto QQ = world.query<Position, Velocity, Force, Mass>();
    QQ.each([&](Position& p, Velocity& v, Force& f, Mass& m) { 
      total_p += p.x + p.y + p.z + v.x + v.y + v.z; 
      v.x += f.x * m.value;
      v.y += f.y * m.value;
      v.z += f.z * m.value;
    });
  }
  return total_p;
}

#endif


int main() {
  LOG_INFO("Start AOS/SOA/ECS benchmark...");

  LOG_INFO("total_p is {} from test_AOS", test_AOS());
  LOG_INFO("total_p is {} from test_SOA", test_SOA());
  // LOG_INFO("total_p is {} from test_neo_ecs", test_NEO_ECS());
  // LOG_INFO("total_p is {} from test_ECS", test_ECS());
  // LOG_INFO("total_p is {} from test_FLECS", test_FLECS());

  return 0;
}