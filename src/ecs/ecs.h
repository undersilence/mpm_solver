#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace ecs {

// type erase
struct ColumnBase {};

template <class T> struct Column : ColumnBase {
  std::vector<T> elements;
};

using EcsId = uint64_t;
using EntityId = uint64_t;
using ComponentId = uint64_t;
using ArchetypeId = uint64_t;
// storage types identifiers
using Type = std::vector<ComponentId>;
struct ecs_ids_hash_t {
  uint64_t operator()(const std::vector<uint64_t>& v) const {
    uint64_t hash = v.size();
    for (auto& x : v)
      hash ^= 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

// maybe a dummy archetype is needed
struct Archetype {
  ArchetypeId id;
  Type type;
  std::vector<std::unique_ptr<ColumnBase>> components;
  std::unordered_map<ComponentId, struct ArchetypeEdge> edges;
};

struct ArchetypeEdge {
  struct Archetype& add;
  struct Archetype& remove;
};

struct Entity {
  int entity_id;
  template <class... Comps> Entity& add() { return *this; }

  // Get component of type Comp
  template <class Comp> auto get() -> Comp { // transform comp to component id
  }

private:
  struct World& world; // must init by world
};

struct World {

  uint64_t ecs_id_count = 0;
  struct Record {
    Archetype& archetype;
    size_t row;
  };
  struct ArchetypeRecord {
    size_t column;
  };
  // Find an archetype by its list of component ids
  std::unordered_map<Type, Archetype, ecs_ids_hash_t> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<EntityId, Record> entity_index;
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
  // Record in component index with component column for archetype
  std::unordered_map<ComponentId, ArchetypeMap> component_index;
  std::unordered_map<std::string, ComponentId> component_map; // component signature to id

  // initialize world
  void init() {}

  EntityId entity() {
    // add empty entity into empty archetype
  }

  // create or get new component_id
  template <class Comp> ComponentId component() {
    auto comp_sig = __PRETTY_FUNCTION__;
    printf("comp_sig: %s\n", comp_sig);
    auto iter = component_map.find(comp_sig);
    if (iter == component_map.end()) {
      return component_map[comp_sig] = ++ecs_id_count;
    } else {
      return iter->second;
    }
  };
};

} // namespace ecs