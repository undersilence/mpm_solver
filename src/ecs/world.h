#pragma once
#include <unordered_map>
#include <vector>

namespace ecs {

struct World {
  // type erase
  struct Column {
    void* elements;
    size_t size;
    size_t count;
  };

  using EntityId = uint64_t;
  using ComponentId = uint64_t;
  using ArchetypeId = uint64_t;
  // storage types identifiers
  using Type = std::vector<ComponentId>;

  struct Archetype {
    ArchetypeId id;
    Type type;
    std::vector<Column> components;
    std::unordered_map<ComponentId, struct ArchetypeEdge> edges;
  };

  struct ArchetypeEdge {
    struct Archetype& add;
    struct Archetype& remove;
  };

  // Find an archetype by its list of component ids
  std::unordered_map<Type, Archetype> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<EntityId, Archetype&> entity_index;
  // Record in component index with component column for archetype
  struct ArchetypeRecord {
    size_t column;
  };
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
  std::unordered_map<ComponentId, ArchetypeMap> component_index;
};

} // namespace ecs