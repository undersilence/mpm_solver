#pragma once
#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

#include "forward.hpp"
#include "diy/vec.hpp"
// #include "utils/timer.hpp"

namespace sim::ecs {

using ecs_id_t = uint64_t;
// storage types identifiers
using Type = std::vector<ecs_id_t>;

struct TypeInfo {
  size_t size;
};

// another name: Table
struct Archetype {

  using Column = vec_core_t;

  struct World& world;
  ecs_id_t id;
  const Type type;
  std::vector<Column> components;
  std::unordered_map<ecs_id_t, Archetype&> add_archetypes;
  std::unordered_map<ecs_id_t, Archetype&> del_archetypes;

  std::unordered_map<ecs_id_t, size_t> entity_to_row;
  std::unordered_map<size_t, ecs_id_t> row_to_entity;

  std::unordered_map<size_t, ecs_id_t> col_to_comp;
  std::unordered_map<ecs_id_t, size_t> comp_to_col;

  Archetype() = delete;
  Archetype(World& world, ecs_id_t id, const Type& type);

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

    other.id = std::numeric_limits<uint64_t>::max();
    other.components.clear();
    other.add_archetypes.clear();
    other.del_archetypes.clear();
  }
  bool operator==(const Archetype& other) const { return id == other.id; }

  auto row() const { return row_to_entity.size(); }
  auto col() const { return type.size(); }

  //  [[nodiscard]] std::vector<std::any> get_entity_row(ecs_id_t entity_id) const {
  //    assert(entity_to_row.contains(entity_id) && "entity not exists in this archetype.");
  //    auto row = entity_to_row.at(entity_id);
  //    std::vector<std::any> row_data;
  //    for (auto& column : components) {
  //      row_data.emplace_back(column[row]);
  //    }
  //    return row_data;
  //  }

  //  std::vector<std::reference_wrapper<std::any>> get_entity_row(ecs_id_t entity_id) {
  //    assert(entity_to_row.contains(entity_id) && "entity not exists in this archetype.");
  //    auto row = entity_to_row.at(entity_id);
  //    std::vector<std::reference_wrapper<std::any>> row_data;
  //    for (auto& column : components) {
  //      row_data.emplace_back(std::ref(column[row]));
  //    }
  //    return row_data;
  //  }

  void add_entity_row(ecs_id_t entity_id, void** row_data) {
    assert(!entity_to_row.contains(entity_id) && "entity already exists");
    //    assert(row_data.size() == type.size() &&
    //           "row_data size should equals to the size of archetype:: type list.");
    auto nxt_row = row_to_entity.size();
    entity_to_row[entity_id] = nxt_row;
    row_to_entity[nxt_row] = entity_id;
    for (size_t i = 0; i < type.size(); ++i) {
      components[i].push_back(row_data[i]);
    }
  }

  //  void set_entity_row(ecs_id_t entity_id, std::vector<std::any>&& row_data) {
  //    assert(row_data.size() == type.size() &&
  //           "row_data size should equals to the size of archetype:: type list.");
  //    if (entity_to_row.contains(entity_id)) {
  //      // entity exists then do update
  //      auto row = entity_to_row.at(entity_id);
  //      for (size_t i = 0; i < row_data.size(); ++i) {
  //        components[i][row].swap(row_data[i]);
  //      }
  //    } else {
  //      add_entity_row(entity_id, std::move(row_data));
  //    }
  //  }

  void del_entity_row(ecs_id_t entity_id) {
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
  void swap_entity_row(ecs_id_t entity_x, ecs_id_t entity_y) {
    auto& row_x = entity_to_row[entity_x];
    auto& row_y = entity_to_row[entity_y];
    // swap each component
    for (auto& column : components) {
      column.swap(row_x, row_y); // swap two indices
    }
    std::swap(row_to_entity[row_x], row_to_entity[row_y]);
    std::swap(row_x, row_y);
  }
};

struct ecs_ids_hash_t {
  size_t operator()(const Type& v) const {
    size_t hash = v.size();
    for (auto& x : v)
      hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct ecs_archetype_equal_t {
  bool operator()(const Type& lhs, const Type& rhs) const { return lhs == rhs; }
};

template <class... Comps> struct Query;

struct World {

  struct Record { // entity query caching
    Archetype& archetype;
    Record() = delete;
    Record(Record&& other) noexcept : archetype(other.archetype) {}
    Record(Archetype& archetype) : archetype(archetype) {}
  };

  struct ArchetypeRecord {
    size_t column;
  };

  uint64_t ecs_id_count = 0;
  // Find an archetype by its list of component ids, where archetype stored
  std::unordered_map<Type, Archetype, ecs_ids_hash_t, ecs_archetype_equal_t> archetype_index;
  // Find the archetype for an entity
  std::unordered_map<ecs_id_t, Record> entity_index;
  // Used to lookup components in archetypes
  using ArchetypeMap = std::unordered_map<ecs_id_t, ArchetypeRecord>;
  // Record in component index with component column for archetype
  std::unordered_map<ecs_id_t, ArchetypeMap> component_index;
  std::unordered_map<std::string, ecs_id_t> signature_to_id; // component signature to id
  std::unordered_map<ecs_id_t, Traits> component_traits;

  // initialize world
  World() { init(); }
  void init();
  struct Entity entity();
  // return the reference of achetype based on these components
  Archetype& archetype(const Type& type);
  Archetype& add_to_archetype(Archetype& src_archetype, ecs_id_t component_id);
  Archetype& del_to_archetype(Archetype& src_archetype, ecs_id_t component_id);
  void add_component(ecs_id_t entity_id, ecs_id_t component_id, void* value);
  void* get_component(ecs_id_t entity_id, ecs_id_t component_id);
  void set_component(ecs_id_t entity_id, ecs_id_t component_id, void* value);
  std::pair<bool, World::ArchetypeMap::iterator> has_component(ecs_id_t entity_id,
                                                               ecs_id_t component_id);
  void del_component(ecs_id_t entity_id, ecs_id_t component_id);

  // multiple operations, small vectors suitable for passing-by-copy
  void add_components(ecs_id_t entity_id, std::vector<ecs_id_t> component_ids, void** values);
  void set_components(ecs_id_t entity_id, std::vector<ecs_id_t> component_ids, void** values);

  // return tuple of components' references
  template <size_t N, typename Indices = std::make_index_sequence<N>>
  auto get_components(ecs_id_t entity_id, std::array<ecs_id_t, N> component_ids) {
    return get_components_impl(entity_id, component_ids, Indices{});
  }
  template <size_t N, size_t... Is>
  auto get_components_impl(ecs_id_t entity_id, std::array<ecs_id_t, N> component_ids,
                           std::index_sequence<Is...>) {
    return std::forward_as_tuple(get_component(entity_id, component_ids[Is])...);
  }

  // create or get new component_id, no related archetypes are created here
  template <class Comp> ecs_id_t get_id() {
    auto comp_sig = signature<Comp>();
    auto iter = signature_to_id.find(comp_sig);
    if (iter == signature_to_id.end()) {
      printf("create component(%s) as id(%llu).\n", comp_sig, ecs_id_count);
      // save type traits
      component_traits.emplace(ecs_id_count, create_traits<Comp>());
      // storage the default value created by 'Component' type
      signature_to_id[comp_sig] = ecs_id_count;
      return ecs_id_count++;
    }
    return iter->second;
  }

  template <class... Comps> auto query() { return Query<Comps...>(*this); }

  template <class Comp> [[nodiscard]] inline const char* signature() const { return FUNCTION_SIG; }
};
  
struct Entity {
  World& world;
  ecs_id_t id;
  Entity(World& world, ecs_id_t id) : world(world), id(id) {}
  Entity() = delete;
  Entity(const Entity&) = default;
  Entity(Entity&&) = default;

  // dirty implementation
  template <class... Comps, size_t... Is> void** malloc_default_values(std::index_sequence<Is...>) {
    // only malloc, no free
    void** values = (void**)malloc(sizeof...(Comps) * sizeof(void*));
    ((values[Is] = new Comps()), ...);
    return values;
  }

  template <class... Comps, size_t... Is>
  bool free_default_values(std::index_sequence<Is...>, void** values) {
    ((delete (Comps*)values[Is]), ...);
    free(values);
  }

  template <class... Comps, size_t... Is>
  void** malloc_values(std::index_sequence<Is...>, Comps&&... _values) {
    void** values = (void**)malloc(sizeof...(Comps) * sizeof(void*));
    ((values[Is] = &_values), ...);
    return values;
  }

  // give default values for all, or not (default construct for all).
  template <class... Comps> Entity& add() {
    auto values = malloc_default_values<Comps..., std::make_index_sequence<sizeof...(Comps)>{}>();
    world.add_components(id, {{world.get_id<Comps>()...}}, values);
    free_default_values(values);
    return *this;
  }

  template <class... Comps> Entity& add(Comps&&... value) {
    auto values = malloc_values<Comps...>(std::make_index_sequence<sizeof...(Comps)>{},
                                          std::forward<Comps>(value)...);
    world.add_components(id, {{world.get_id<Comps>()...}}, values);
    free(values);
    return *this;
  }

  template<class Comp, class... Args> Entity& emplace(Args&&... args) {

  }

  template <class Comp> Entity& add(Comp&& value = Comp{}) {
    auto comp_id = world.get_id<Comp>();
    world.add_component(id, comp_id, std::forward<Comp>(value));
    return *this;
  }

  template <class... Comps> auto get() {
    return std::forward_as_tuple(world.get_component(id, world.get_id<Comps>())...);
  }

  template <class Comp> Comp& get() {
    auto comp_id = world.get_id<Comp>();
    return *(Comp*)(world.get_component(id, comp_id));
  }

  template <class... Comps> Entity& set(Comps&&... value) {
    auto values = malloc_values<Comps...>(std::make_index_sequence<sizeof...(Comps)>{},
                                          std::forward<Comps>(value)...);
    world.set_components(id, {{world.get_id<Comps>()...}}, values);
    free(values);
    return *this;
  }

  template <class Comp> Entity& set(Comp&& value = Comp{}) {
    auto comp_id = world.get_id<Comp>();
    world.set_component(id, comp_id, &value);
    return *this;
  }

  template <class Comp> bool has() {
    auto comp_id = world.get_id<Comp>();
    return world.has_component(id, comp_id).first;
  }
};

// wrapper
struct Component {
  struct World& world;
  ecs_id_t id; //
  // maybe has a signature
  // std::string_view signature;
  Component(World& world, ecs_id_t id) : world(world), id(id) {}
};

// Query as container form
template <class... Comps> struct Query {
  Query(World& world) : world(world) {
    //    FUNCTION_TIMER();
    comp_ids = std::vector<ecs_id_t>{{(world.get_id<Comps>())...}};
    auto type = comp_ids;
    std::sort(type.begin(), type.end());
    auto archetype_iter = world.archetype_index.find(type);
    if (archetype_iter != world.archetype_index.end()) {
      init_related_archetypes(archetype_iter->second);
    }
  }

  // recursively init related archetypes
  void init_related_archetypes(Archetype& archetype) {
    if (archetype.row() > 0) {
      auto cols = std::vector<size_t>();
      related_archetypes.emplace_back(archetype);
      for (auto comp_id : comp_ids) {
        cols.emplace_back(world.component_index[comp_id][archetype.id].column);
      }
      archetypes_columns.emplace_back(std::move(cols));
    }
    for (auto& archetype_pair : archetype.add_archetypes) {
      init_related_archetypes(archetype_pair.second);
    }
  }

  // Iterator definition
  struct Iterator {
    using val_t = std::tuple<Comps...>;
    using ref_t = std::tuple<Comps&...>;
    using ptr_t = std::tuple<Comps*...>;
    Query& self;
    size_t archetype_idx;
    size_t row_idx;
    static constexpr size_t N = sizeof...(Comps);

    Iterator(Query& query, size_t archetype_idx, size_t row_idx)
        : self(query), archetype_idx(archetype_idx), row_idx(row_idx) {}

    Iterator& operator++() {
      row_idx++;
      while (row_idx == row_size()) {
        row_idx = 0;
        archetype_idx++;
      }
      return *this;
    };

    bool operator==(const Iterator& rhs) {
      return &self == &rhs.self && archetype_idx == rhs.archetype_idx && row_idx == rhs.row_idx;
    }

    size_t row_size() {
      return archetype_idx >= self.related_archetypes.size() || archetype_idx < 0
                 ? std::numeric_limits<size_t>::max()
                 : self.related_archetypes[archetype_idx].get().row();
    };

    Iterator& advance(ptrdiff_t step) {
      if (step > 0) {
        // assume that there is a inf row dummy archetype at end of related_archetypes
        while (step > 0 && row_idx + step >= row_size()) {
          step -= row_size() - row_idx;
          row_idx = 0;
          archetype_idx++;
        }
      } else if (step < 0) {
        // assert(false && "negative advance not implement yet!");
        while (step < 0 && (ptrdiff_t)row_idx < step) {
          step += row_idx + 1;
          archetype_idx--;
          row_idx = row_size() - 1; // assume that all row size > 0 in related archetypes
        }
      }
      row_idx += step;
      return *this;
    }

    ref_t operator*() { return get<>(std::make_index_sequence<sizeof...(Comps)>{}); }

    template <size_t... Is> inline ref_t get(std::index_sequence<Is...>) {
      return std::forward_as_tuple(get<Comps, Is>()...);
    }

    // should not use auto as return val, case any_cast<T> could return pointer here.
    template <typename Comp, size_t I> inline Comp& get() {
      auto& archetype_ref = self.related_archetypes[archetype_idx];
      auto& col_idx = self.archetypes_columns[archetype_idx][I];
      // printf("Get value at (%d, %d) of archetype_%d (type: %s)\n", col_idx, row_idx,
      // archetype_ref.get().id, self.world.signature<Comp>());
      return *(Comp*)archetype_ref.get().components[col_idx][row_idx];
    }
  };

  Iterator begin() { return Iterator(*this, 0, 0); }
  Iterator end() { return Iterator(*this, related_archetypes.size(), 0); }

  constexpr auto col() const { return comp_ids.size(); }
  size_t size() {
    size_t total_count = 0;
    for (auto& archetype : related_archetypes) {
      total_count += archetype.get().row();
    }
    return total_count;
  }

  World& world;
  std::vector<std::reference_wrapper<Archetype>> related_archetypes;
  std::vector<ecs_id_t> comp_ids;
  std::vector<std::vector<size_t>> archetypes_columns;
};

} // namespace sim::ecs