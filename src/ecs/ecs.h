#pragma once
#include "forward.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <optional>
#include <stack>
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
};

// another name: Table
struct Archetype {
  struct World& world;
  ArchetypeId id;
  const Type type;
  std::vector<std::vector<std::any>> components;
  std::unordered_map<ComponentId, Archetype&> add_archetypes;
  std::unordered_map<ComponentId, Archetype&> del_archetypes;

  std::unordered_map<EntityId, size_t> entity_to_row;
  std::unordered_map<size_t, EntityId> row_to_entity;

  std::unordered_map<size_t, ComponentId> col_to_comp;
  std::unordered_map<ComponentId, size_t> comp_to_col;

  Archetype() = delete;
  Archetype(World& world, ArchetypeId id, const Type& type) : world(world), id(id), type(type) {
    components.resize(type.size());
    for (auto& column : components) {
      column.clear();
    }
  };

  Archetype(const Archetype&) = delete;
  // Only move operation is allowed
  Archetype(Archetype&& other) noexcept : world(other.world), id(other.id), type(other.type) {
    components.swap(other.components);
    add_archetypes.swap(other.add_archetypes);
    del_archetypes.swap(other.del_archetypes);
    // move entity <=> row bimap
    entity_to_row.swap(other.entity_to_row);
    row_to_entity.swap(other.row_to_entity);
    // move comp <=> column bimap
    comp_to_col.swap(other.comp_to_col);
    col_to_comp.swap(other.col_to_comp);

    other.id = -1;
    other.components.clear();
    other.add_archetypes.clear();
    other.del_archetypes.clear();
  }
  bool operator==(const Archetype& other) const { return id == other.id; }

  [[nodiscard]] std::vector<std::any> get_entity_row(EntityId entity_id) const {
    assert(entity_to_row.contains(entity_id) && "entity not exists in this archetype.");
    auto row = entity_to_row.at(entity_id);
    std::vector<std::any> row_data;
    for (auto& column : components) {
      row_data.emplace_back(column[row]);
    }
    return row_data;
  }

  //  std::vector<std::reference_wrapper<std::any>> get_entity_row(EntityId entity_id) {
  //    assert(entity_to_row.contains(entity_id) && "entity not exists in this archetype.");
  //    auto row = entity_to_row.at(entity_id);
  //    std::vector<std::reference_wrapper<std::any>> row_data;
  //    for (auto& column : components) {
  //      row_data.emplace_back(std::ref(column[row]));
  //    }
  //    return row_data;
  //  }

  void add_entity_row(EntityId entity_id, std::vector<std::any>&& row_data) {
    assert(!entity_to_row.contains(entity_id) && "entity already exists");
    assert(row_data.size() == type.size() &&
           "row_data size should equals to the size of archetype:: type list.");
    auto nxt_row = row_to_entity.size();
    entity_to_row[entity_id] = nxt_row;
    row_to_entity[nxt_row] = entity_id;
    for (int i = 0; i < type.size(); ++i) {
      components[i].emplace_back(std::move(row_data[i]));
    }
  }

  void set_entity_row(EntityId entity_id, std::vector<std::any>&& row_data) {
    assert(row_data.size() == type.size() &&
           "row_data size should equals to the size of archetype:: type list.");
    if (entity_to_row.contains(entity_id)) {
      // entity exists then do update
      auto row = entity_to_row.at(entity_id);
      for (size_t i = 0; i < row_data.size(); ++i) {
        components[i][row].swap(row_data[i]);
      }
    } else {
      add_entity_row(entity_id, std::move(row_data));
    }
  }

  void del_entity_row(EntityId entity_id) {
    if (!row_to_entity.empty() && !entity_to_row.contains(entity_id)) {
      // entity not exists
      return;
    }
    // try swap entity_row to the end.
    auto last_row = row_to_entity.size() - 1;
    auto last_entity = row_to_entity[last_row];
    if (last_entity != entity_id) {
      swap_entity_row(entity_id, last_entity);
    }
    // remove last entity
    entity_to_row.erase(entity_id);
    row_to_entity.erase(last_row);
    for (auto& column : components) {
      column.pop_back();
    }
  }

  // further entity sorting support
  void swap_entity_row(EntityId entity_x, EntityId entity_y) {
    auto& row_x = entity_to_row[entity_x];
    auto& row_y = entity_to_row[entity_y];
    // swap each component
    for (auto& column : components) {
      std::swap(column[row_x], column[row_y]);
    }
    std::swap(row_to_entity[row_x], row_to_entity[row_y]);
    std::swap(row_x, row_y);
  }
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

// Query iterator
struct Query {};

struct World {

  struct Record { // entity query caching
    Archetype& archetype;
    size_t row;
    Record() = delete;
    Record(Record&& other) noexcept : archetype(other.archetype), row(other.row) {}
    Record(Archetype& archetype, size_t row) : archetype(archetype), row(row) {}
  };

  struct ArchetypeRecord {
    size_t column;
  };

  uint64_t ecs_id_count = 0;
  // Find an archetype by its list of component ids, where archetype stored
  std::unordered_map<Type, Archetype, ecs_ids_hash_t, ecs_archetype_equal_t> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<EntityId, Record> entity_index;
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
  // Record in component index with component column for archetype
  std::unordered_map<ComponentId, ArchetypeMap> component_index;
  std::unordered_map<std::string_view, ComponentId> signature_to_id; // component signature to id
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
    auto comp_sig = signature<Comp>();
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

  template <class... Comps> void query() {
    // dfs on archetypes' graph
    std::vector<ComponentId> comp_ids = {get_id<Comps>()...};
    auto sorted_comp_ids = comp_ids;
    std::sort(sorted_comp_ids.begin(), sorted_comp_ids.end());

    auto archetype_iter = archetype_index.find(sorted_comp_ids);
    if (archetype_iter == archetype_index.end())
      return;

    auto& archetype_node = archetype_iter->second;
    printf("archetype %llu entity size: %llu\n", archetype_node.id,
           archetype_node.entity_to_row.size());
    // start dfs
    for (auto& node : archetype_node.add_archetypes) {
      printf("archetype %llu entity size: %llu\n", node.second.id,
             node.second.entity_to_row.size());
    }
  }

  template <class Comp> inline const char* signature() const { return FUNC_SIG; }
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
    world.add_component(id, comp_id, std::forward<Comp>(value));
    return *this;
  }

  template <class Comp> Comp& get() {
    auto comp_id = world.get_id<Comp>();
    return std::any_cast<Comp&>(world.get_component(id, comp_id));
  }

  template <class Comp> Entity& set(Comp&& value = Comp()) {
    auto comp_id = world.get_id<Comp>();
    world.set_component(id, comp_id, std::forward<Comp>(value));
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