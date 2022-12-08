#pragma once
#include "forward.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ecs {

using EcsId = uint64_t;
using EntityId = EcsId;
using ComponentId = EcsId;
using ArchetypeId = EcsId;
// storage types identifiers
using Type = std::vector<ComponentId>;

struct TypeInfo {
  size_t size;
  std::any default_value;
};

// maybe a dummy archetype is needed
struct Archetype {
  struct World& world;
  ArchetypeId id;
  Type type;
  std::vector<std::vector<std::any>> components;
  std::unordered_map<ComponentId, Archetype&> add_archetypes;
  std::unordered_map<ComponentId, Archetype&> del_archetypes;

  Archetype() = delete;
  Archetype(World& world, ArchetypeId id, const Type& type) : world(world), id(id), type(type) {
    components.resize(type.size());
    for (auto& column : components) {
      column.clear();
    }
  };
  Archetype(const Archetype&) = delete;
  // Only move operation is allowed
  Archetype(Archetype&& other) : world(other.world), id(other.id), type(other.type) {
    components.swap(other.components);
    add_archetypes.swap(other.add_archetypes);
    del_archetypes.swap(other.del_archetypes);
    other.id = -1;
    other.components.clear();
    other.add_archetypes.clear();
    other.del_archetypes.clear();
  }
  bool operator==(const Archetype& other) const { return id == other.id; }
};

struct ecs_ids_hash_t {
  size_t operator()(const Type& v) const {
    size_t hash = v.size();
    for (auto& x : v)
      hash ^= 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct ecs_archetype_equal_t {
  bool operator()(const Type& lhs, const Type& rhs) const { return lhs == rhs; }
};

struct World {

  struct Record {
    Archetype& archetype;
    size_t row;
    Record() = delete;
    Record(Record&& other)  noexcept : archetype(other.archetype), row(other.row) {}
    Record(Archetype& archetype, size_t row) : archetype(archetype), row(row) {}
  };

  struct ArchetypeRecord {
    size_t column;
  };

  uint64_t ecs_id_count = 0;
  // Find an archetype by its list of component ids
  std::unordered_map<Type, Archetype, ecs_ids_hash_t, ecs_archetype_equal_t> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<EntityId, Record> entity_index;
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
  // Record in component index with component column for archetype
  std::unordered_map<ComponentId, ArchetypeMap> component_index;
  std::unordered_map<std::string, ComponentId> signature_to_id; // component signature to id
  // std::unordered_map<ComponentId, std::any> component_default_val;

  // initialize world
  World() { init(); }
  void init();
  struct Entity entity();
  // return the reference of achetype based on these components
  Archetype& archetype(const Type& type);
  Archetype& add_to_archetype(Archetype& src_archetype, ComponentId component_id);
  void add_component(EntityId entity_id, ComponentId component_id, std::any value);
  std::any& get_component(EntityId entity_id, ComponentId component_id);
  void set_component(EntityId entity_id, ComponentId component_id, std::any value);
  std::pair<bool, World::ArchetypeMap::iterator> has_component(EntityId entity_id,
                                                               ComponentId component_id);

  // create or get new component_id, no related archetypes are created here
  template <class Comp> ComponentId get_id() {
    auto comp_sig = FUNC_SIG;
    auto iter = signature_to_id.find(comp_sig);
    if (iter == signature_to_id.end()) {
      printf("Create component: %s\n", comp_sig);
      // storage the default value created by 'Component' type
      // component_default_val.emplace(ecs_id_count, std::move(Comp()));
      signature_to_id[comp_sig] = ecs_id_count;
      return ecs_id_count++; // create temporary proxy
    }
    return iter->second;
  }
};

struct Entity {
  World& world; // must init by world
  EntityId id;
  Entity(World& world, EntityId id) : world(world), id(id) {}
  Entity() = delete;
  Entity(const Entity&) = default;
  Entity(Entity&&) = default;

  template <class Comp> Entity& add(Comp&& value = Comp()) {
    auto comp_id = world.get_id<Comp>();
    world.add_component(id, comp_id, std::move(Comp()));
    return *this;
  }

  template <class Comp>

  Comp& get() {
    auto comp_id = world.get_id<Comp>();
    return std::any_cast<Comp&>(world.get_component(id, comp_id));
  }

  template <class Comp> Entity& set(Comp&& value = Comp()) {
    auto comp_id = world.get_id<Comp>();
    world.set_component(id, comp_id, value);
    return *this;
  }

  template <class Comp> bool has() {
    auto comp_id = world.get_id<Comp>();
    return world.has_component(id, comp_id).first;
  }
};

// wrapper
struct Component {
  ComponentId id; //
  // maybe has a signature
  // std::string_view signature;
  struct World& world;
  Component(struct World& world, ComponentId id) : world(world), id(id) {}
};

} // namespace ecs