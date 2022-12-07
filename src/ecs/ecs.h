#pragma once
#include "forward.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

namespace ecs {

struct World;
struct Entity;
struct Archetype;

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
  std::vector<std::vector<std::any>> components;
  std::unordered_map<ComponentId, Archetype&> add_archetypes;
  std::unordered_map<ComponentId, Archetype&> del_archetypes;

  Archetype() = delete;
  Archetype(World& world, ArchetypeId id, const Type& type) : world(world), id(id), type(type) {
    components.resize(type.size());
  };
  Archetype(World& world, ArchetypeId id, Type&& type) : world(world), id(id), type(type) {
    components.resize(type.size());
  };
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
  std::unordered_map<ComponentId, std::any> component_default;

  World() { init(); }

  // initialize world
  void init() {
    // create root empty archetype, only archetype::id can copy.
    archetype_index.emplace(std::make_pair(Type{}, std::move(archetype(Type{}))));
  }

  EntityId entity() {
    // add empty entity into empty archetype
    EntityId new_entity_id = ecs_id_count++;
    Record record(archetype_index.at(Type{}), -1);
    entity_index.emplace(std::make_pair(new_entity_id, record));
    return new_entity_id;
  }

  Archetype& add_to_archetype(Archetype& src_archetype, ComponentId component_id) {
    // assume that src_archetype not contains this component
    auto type = src_archetype.type;
    // check if dst_archetype is in the cache
    auto archetype_iter = src_archetype.add_archetypes.find(component_id);
    if (archetype_iter == src_archetype.add_archetypes.end()) {
      Archetype& dst_archetype = archetype(type);
      src_archetype.add_archetypes.emplace(component_id, dst_archetype);
      dst_archetype.del_archetypes.emplace(component_id, src_archetype);
    }
    return archetype_iter->second;
  }

  void add_component(EntityId entity_id, ComponentId component_id) {
    Record& record = entity_index.at(entity_id);
    Archetype& src = record.archetype;
    Archetype& dst = add_to_archetype(src, component_id);

    if (record.row == -1) {
      for (int i = 0; i < dst.type.size(); i++) {
        auto comp_id = dst.type[i];
        // append default value of component
        dst.components[i].emplace_back(component_default[comp_id]); 
      }
      return;
    }
    
    for (int i = 0; i < src.type.size(); i++) {
      auto comp_id = src.type[i];
      auto dst_col = component_index[comp_id][dst.id].column;
      auto dst_row = dst.components[dst_col].size();
      dst.components[dst_col].emplace_back(std::move(src.components[i][record.row]));
      src.components[i].erase(src.components[i].begin() + record.row);
      // update entity_index
      std::swap(entity_index[entity_id].archetype, dst);
      entity_index[]
    }
  }

  // create or get new component_id
  template <class Comp> ComponentId component() {
    auto comp_sig = FUNC_SIG;
    printf("comp_sig: %s\n", comp_sig);
    auto iter = signature_to_id.find(comp_sig);
    if (iter == signature_to_id.end()) {
      // storage the default value created by 'Component' type
      component_default[ecs_id_count] = Comp();
      return signature_to_id[comp_sig] = ecs_id_count++;
    }
    return iter->second;
  }

  // return the reference of achetype based on these components
  Archetype& archetype(const Type& type) {
    // check if archetype_index exists
    auto archetype_iter = archetype_index.find(type);
    if (archetype_iter == archetype_index.end()) {
      // if not exists, create new archetype
      Archetype archetype(*this, ecs_id_count++, type);
      auto p = archetype_index.emplace(std::make_pair(type, std::move(archetype)));
      assert(p.second && "emplace new archetype failed.");
      for (size_t i = 0; i < type.size(); ++i) {
        component_index[type[i]][archetype.id] = ArchetypeRecord{i};
      }
      return p.first->second;
    } else {
      return archetype_iter->second;
    }
  }
};

} // namespace ecs