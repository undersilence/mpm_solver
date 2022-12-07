#pragma once
#include "forward.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace ecs {

struct World;
struct Entity;
struct Archetype;

// type erase
struct ColumnBase {};

template <class T> struct Column : ColumnBase {
  std::vector<T> elements;
};

using EcsId = uint64_t;
using EntityId = EcsId;
using ComponentId = EcsId;
using ArchetypeId = EcsId;
// storage types identifiers
using Type = std::vector<ComponentId>;

// maybe a dummy archetype is needed
struct Archetype {
  World& world;
  ArchetypeId id;
  Type type;
  std::vector<std::unique_ptr<ColumnBase>> components;
  std::unordered_map<ComponentId, Archetype&> add_archetypes;
  std::unordered_map<ComponentId, Archetype&> del_archetypes;

  Archetype() = delete;
  Archetype(World& world, ArchetypeId id) : world(world), id(id){};
  Archetype(const Archetype&) = delete;
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

struct ArchetypeEdge {
  Archetype& add;
  Archetype& del;
  ArchetypeEdge() = delete;
  ArchetypeEdge(Archetype& add, Archetype& del) : add(add), del(del) {}
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

struct Entity {
  EntityId id;
  Entity(struct World& world, EntityId id) : world(world), id(id) {}
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
    Record() = delete;
    Record(Archetype& archetype, size_t row) : archetype(archetype), row(row) {}
  };
  struct ArchetypeRecord {
    size_t column;
  };
  // Find an archetype by its list of component ids
  std::unordered_map<Type, Archetype, ecs_ids_hash_t, ecs_archetype_equal_t> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<EntityId, Record> entity_index;
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
  // Record in component index with component column for archetype
  std::unordered_map<ComponentId, ArchetypeMap> component_index;
  std::unordered_map<std::string, ComponentId> signature_to_id; // component signature to id

  World() { init(); }

  // initialize world
  void init() {
    // create root empty archetype, only archetype::id can copy.
    Archetype empty_archetype(*this, ecs_id_count++);
    archetype_index.emplace(std::make_pair(Type{}, std::move(empty_archetype)));
  }

  EntityId entity() {
    // add empty entity into empty archetype
    EntityId new_entity_id = ecs_id_count++;
    Record record(archetype_index.at(Type{}), -1);
    entity_index.emplace(std::make_pair(new_entity_id, record));
    return new_entity_id;
  }

  Archetype& add_to_archetype(Archetype& src_archetype, ComponentId component_id) {
    auto type = src_archetype.type;
    if(insert_component(type, component_id)) {
      src_archetype.add_archetypes.emplace(std::make_pair(component_id, create_archetype()));
    }
  }

  bool add_component(EntityId entity_id, ComponentId component_id) {
    Record& record = entity_index.at(entity_id);
    Archetype& old_archetype = record.archetype;
    // check if the edge has been created previously
    auto archetype_iter = old_archetype.add_archetypes.find(component_id);
    // not created so far
    if (archetype_iter == old_archetype.add_archetypes.end()) {
      Archetype&& new_archetype = create_archetype();
      new_archetype.type = old_archetype.type;
      if (insert_component(new_archetype.type, component_id)) {
        new_archetype.components.resize(new_archetype.type.size());
        move_entity(old_archetype, record.row, new_archetype);
        return true;
      }
    }
    return false;
  }

  // insert component_id into archetype.type, keep increasing order
  bool insert_component(Type& type, ComponentId component_id) {
    auto insert_iter = std::lower_bound(type.begin(), type.end(), component_id);
    if (*insert_iter == component_id) {
      return false; // component can not repeat, already exists!
    } else {
      type.insert(insert_iter, component_id);
    }
  }

  void move_entity(Archetype& src, size_t row, Archetype& dst) {
    for (auto& column_ptr : src.components) {
    }
  }

  // create or get new component_id
  template <class Comp> ComponentId component() {
    auto comp_sig = FUNC_SIG;
    printf("comp_sig: %s\n", comp_sig);
    auto iter = signature_to_id.find(comp_sig);
    if (iter == signature_to_id.end()) {
      return signature_to_id[comp_sig] = ++ecs_id_count;
    } else {
      return iter->second;
    }
  };

  Archetype create_archetype() {
    ArchetypeId archetype_id = ecs_id_count++;
    return {*this, archetype_id};
  }
};

} // namespace ecs