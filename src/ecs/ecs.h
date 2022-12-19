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

  auto row() const { return row_to_entity.size(); }
  auto col() const { return type.size(); }

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

// iterator on Archetypes' DAG
//struct ArchetypeIterator {
//  std::reference_wrapper<Archetype> archetype_ptr;
//  std::unordered_map<ComponentId, Archetype&>::iterator edges_iter;
//
//  ArchetypeIterator(Archetype& archetype) : archetype_ptr(std::ref(archetype)) {
//    auto iter = archetype.add_archetypes.begin();
//  }
//
//  Archetype& operator*() const { return archetype_ptr.get(); }
//
//  ArchetypeIterator& operator++() {
//    edges_iter++;
//    if (archetype_ptr.get().add_archetypes.end() == edges_iter) {
//      // go to next archetype
//    }
//  }
//};

template <class... Comps> struct Query;

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
  std::unordered_map<std::string, ComponentId> signature_to_id; // component signature to id
  // std::unordered_map<ComponentId, std::any> component_default_val;

  // initialize world
  World() { init(); }
  void init();
  struct Entity entity();
  // return the reference of achetype based on these components
  Archetype& archetype(const Type& type);
  Archetype& add_to_archetype(Archetype& src_archetype, ComponentId component_id);
  Archetype& del_to_archetype(Archetype& src_archetype, ComponentId component_id);
  void add_component(EntityId entity_id, ComponentId component_id, std::any value);
  std::any& get_component(EntityId entity_id, ComponentId component_id);
  void set_component(EntityId entity_id, ComponentId component_id, std::any value);
  std::pair<bool, World::ArchetypeMap::iterator> has_component(EntityId entity_id,
                                                               ComponentId component_id);
  void del_component(EntityId entity_id, ComponentId component_id);

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

//  template <class... Comps> auto query() {
//    return Query<Comps...>(*this);
//  }

  template <class Comp> [[nodiscard]] inline const char* signature() const { return FUNC_SIG; }
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


// Query as container form
template <class... Comps> struct Query {
  Query(World& world) : world(world) {
    comp_ids = std::vector<ComponentId> {{ (world.get_id<Comps>())...} };
    auto type = comp_ids;
    std::sort(type.begin(), type.end());
    auto archetype_iter = world.archetype_index.find(type);
    if (archetype_iter != world.archetype_index.end()) {
      init_related_archetypes(archetype_iter->second);
    }
  }

  auto col() const {return comp_ids.size();}

  // recursively init related archetypes
  void init_related_archetypes(Archetype& archetype) {
    if (archetype.row() <= 0) {
      return;
    }
    related_archetypes.emplace_back(archetype);
    for (auto& next_archetype : archetype.add_archetypes) {
      init_related_archetypes(next_archetype.second);
    }
  }

//  struct Iterator {
//    using iterators_t = std::tuple<typename std::vector<Comps>::iterator...>;
//    using reference_t = std::tuple<Comps&...>;
//
//    std::reference_wrapper<Archetype> curr_archetype;
//    size_t curr_row, max_row;
//    Query& query;
//    iterators_t iterators;
//
//    reference_t get() {
//      return std::apply([&](auto&&... args) { return reference_t((*args)...); }, iterators);
//    }
//
//    Iterator& advance(ptrdiff_t step_size) {
//      std::apply([&](auto&&... iter) { (iter += step_size,...); }, iterators);
//    }
//  };

  size_t size() {
    size_t total_count = 0;
    for (auto& archetype : related_archetypes) {
      total_count += archetype.get().row();
    }
  }

  World& world;
  std::vector<std::reference_wrapper<Archetype>> related_archetypes;
  std::vector<ComponentId> comp_ids;
};

} // namespace ecs