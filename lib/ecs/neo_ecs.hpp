#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "forward.hpp"

namespace sim {

namespace utils {

template <class TDerived, class TBase>
auto down_cast(TBase* ptr) -> TDerived* {
  return static_cast<TDerived*>(ptr);
}

template <class TVec>
struct vec_hash_t {
  size_t operator()(const TVec& v) const {
    size_t hash = v.size();
    for (auto& x : v) hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

template <class TVec>
struct vec_equal_t {
  bool operator()(const TVec& lhs, const TVec& rhs) const { return lhs == rhs; }
};

template <class Ty>
constexpr static inline const char* id() {
  return FUNCTION_SIG;
}

template <class Ty>
struct iterator_chain_t {
  std::vector<Ty> begin_iters;
  std::vector<Ty> end_iters;
  Ty curr_iter;

  template <class Ty0, class Ty1, class... Tys>
  inline void reset(Ty0&& begin_iter, Ty1&& end_iter, Tys&&... args) {
    begin_iters.emplace_back(begin_iter);
    end_iters.emplace_back(end_iter);
    reset(args...);
  }

  template <class Ty_vec, class... Tys>
  inline void reset(Ty_vec&& vec, Tys&&... args) {
    begin_iters.emplace_back(std::begin(vec));
    end_iters.emplace_back(std::end(vec));
    reset(args...);
  }
};

}  // namespace utils

namespace data {

struct IStorage {
 public:
  virtual ~IStorage() {}
  virtual void extend_one(){};
  virtual void shrink_one(){};

 public:
  // NOTE: use mimic() to create same_type empty instance
  virtual IStorage* create_mimic() = 0;
  virtual size_t size() const = 0;
  virtual void swap(size_t i, size_t j) = 0;
  virtual void* operator[](size_t offset) = 0;
  virtual void move(size_t src_idx, IStorage& dst, size_t dst_idx) = 0;
};

template <typename Ty>
struct Array : IStorage {
 public:
  std::vector<Ty> inner_array;

  Array() = default;
  Array(Array const& array) = delete;

  IStorage* create_mimic() override { return new Array<Ty>(); }

  void swap(size_t i, size_t j) override { std::swap(inner_array[i], inner_array[j]); }

  void move(size_t src_idx, IStorage& dst_array, size_t dst_idx) override {
    auto& typed_array = static_cast<Array<Ty>&>(dst_array);
    typed_array.insert(dst_idx, std::move(inner_array[src_idx]));
  }

  template <class _Ty>
  auto append(_Ty&& val) {
    return inner_array.emplace_back(val);
  }

  Ty& at(size_t i) { return inner_array.at(i); }

  size_t size() const override { return inner_array.size(); }

  void* operator[](size_t idx) override { return &inner_array[idx]; }

  void extend_one() override {
    if constexpr (std::is_default_constructible_v<Ty>) {
      append(std::move(Ty()));
    } else {
      assert(false && "value must be set properly for non_default_constructible class");
    }
  }

  void shrink_one() override { inner_array.pop_back(); }

  void insert(size_t pos, Ty&& val) { inner_array.insert(inner_array.begin() + pos, val); }
};
}  // namespace data

namespace ecs {

using Id = uint32_t;
using Tag = std::vector<Id>;

struct Entity {
 public:
  template <typename... Tys>
  Entity& set(Tys&&... args);

  template <typename... Tys>
  Entity& add(Tys&&... args);

  template <typename... Tys>
  bool has();

  template <typename... Tys>
  std::tuple<Tys&...> get();

  template <typename... Tys>
  Entity& del();

  Entity(Id eid, struct World* world);

 public:
  Id eid;
  World* world;
};

struct Table {
 public:
  Id tid;
  struct World* world;

 private:
  std::vector<Id> col2id;
  std::vector<Id> row2id;
  std::unordered_map<Id, size_t> id2col;
  std::unordered_map<Id, size_t> id2row;
  std::vector<data::IStorage*> data;

 public:
  Table(Id tid, World* world);

  template <class... Ty>
  static Table creator(Id tid, World* world);

  // deduce from existing table... mimic their rows to avoid explicit type_traits require.
  template <bool is_add_or_del, class... Ty>
  static Table creator(Id tid, Table const& src_table);

  ~Table();
  Table() = default;
  Table(Table&& table) = default;
  Table(Table const&) = delete;
  Table& operator=(Table&& table) = default;
  Table& operator=(Table const& table) = delete;

  template <class... Idx>
  void add_cols(Idx... id);

  template <class... Ty>
  void add_rows();

  template <class... Ty>
  void del_rows();

  // use this function to create mimic empty row instance
  void mimic_rows(Table const& other_table);

  size_t rows() const { return row2id.size(); }
  size_t cols() const { return col2id.size(); }
  bool ready() const;

  data::IStorage& row(size_t idx) const;

  // Helper: get row by type
  template <class Ty>
  data::Array<Ty>& row() const;

  Tag unsort_tag() const { return row2id; };

  template <class... Ty>
  std::tuple<Ty&...> at(Id id);

  template <class... Ty>
  void set(Id id, Ty&&... val);

  bool has(Id id) const;

  template <class... Ty>
  bool has() const;

  template <class... Ty>
  void add(Id id, Ty&&... val);

  void del(Id id);

  void move(Id id, Table& dst_table);

  template <class... Ty, class TFn>
  void for_each_col(TFn&& func);

  template <class Ty>
  std::vector<Ty>& get_row();

  template <class Ty>
  data::Array<Ty>& typed_row() const;

 private:
  void _append_empty_if_needed();

  static void _move_elemt(Table& src_table, size_t src_row, size_t src_col, Table& dst_table, size_t dst_row, size_t dst_col);

  template <class Ty>
  data::Array<Ty>& _typed_row(size_t row) const;

  template <class Ty>
  void _del_row();

  template <class Ty>
  void _add_row();

  void _del_col(Id id);

  void _add_col(Id id);
};
using Archetype = Table;

template <class... Ty>
struct TableView {
  using val_t = std::tuple<Ty...>;
  using ref_t = std::tuple<Ty&...>;
  using cref_t = std::tuple<Ty const&...>;

  struct view_iterator {
    using value_type = val_t;
    using reference_type = ref_t;
    using difference_type = ptrdiff_t;
    using vec_iters_t = std::tuple<typename std::vector<Ty>::iterator...>;

    vec_iters_t vec_iters;
    difference_type offset;

    view_iterator& advance(difference_type step) {
      offset += step;
      std::apply([step](auto&&... vec_iter) { ((vec_iter += step), ...); }, vec_iters);
      return *this;
    }

    reference_type operator*() {
      return std::apply([](auto&&... vec_iter) { return std::forward_as_tuple(*vec_iter...); }, vec_iters);
    }

    view_iterator& operator++() { return advance(1); }

    bool operator<(view_iterator const& rhs) const { return offset < rhs.offset; }

    bool operator!=(view_iterator const& rhs) const { return (*this < rhs) || !(rhs < *this); }
  };

  using vec_pack_t = std::tuple<std::vector<Ty>&...>;
  using cvec_pack_t = std::tuple<std::vector<Ty> const&...>;

  size_t vec_size;
  vec_pack_t vec_pack;

  TableView(Table& table) : vec_size(table.cols()), vec_pack(table.get_row<Ty>()...) {}

  template <class TFunc>
  void for_each(TFunc&& func) {
    for (decltype(vec_size) idx = 0; idx < vec_size; ++idx) {
      std::apply([idx, &func](auto&&... data_row) { func(data_row[idx]...); }, vec_pack);
    }
  }

  view_iterator begin() { return iter(0); }

  view_iterator end() { return iter(vec_size); }

  view_iterator iter(ptrdiff_t offset) {
    return view_iterator{.vec_iters = std::apply([offset](auto&&... vec) { return std::forward_as_tuple((std::begin(vec) + offset)...); }, vec_pack), .offset = offset};
  }
};

template <class... Ty>
struct Query {
 public:
  using val_t = std::tuple<Ty...>;
  using ref_t = std::tuple<Ty&...>;
  using cref_t = std::tuple<Ty const&...>;

  Query(struct World*);

  void initialize();
  template <class TFn>
  void for_each(TFn&& func);

 private:
  struct World* world;
  std::vector<TableView<Ty...>> views;
};

struct World {
 public:
  template <class... Ty>
  using Map = std::unordered_map<Ty...>;

  template <class... Ty>
  Entity& entity();

  template <class... Ty>
  Entity& entity(Ty&&... args);

  template <class... Ty>
  Query<Ty...> query();

  template <class... Tys>
  void set_components(Id eid, Tys&&... value);

  template <class... Tys>
  void add_components(Id eid, Tys&&... value);

  template <class... Tys>
  void del_components(Id eid);

  template <class... Tys>
  void has_components(Id eid);

  template <class... Tys>
  std::tuple<Tys&...> get_components(Id eid);

 public:
  template <class... Tys>
  Archetype& add_to_archetype(Archetype const& src);

  template <class... Tys>
  Archetype& del_from_archetype(Archetype const& src);

  template <bool is_add_or_del, class... Ty>
  Archetype& deduce_archetype(Archetype const& src_archetype);

  template <class... Ty>
  Archetype& deduce_archetype();

 public:
  Map<Id, Archetype&> entity_records;

  Map<Tag, Archetype&, utils::vec_hash_t<Tag>, utils::vec_equal_t<Tag>> tag_records;

 private:
  void _move_entity(Id eid, Archetype& src_archetype, Archetype& dst_archetype);

  template <class Ty>
  Archetype& _add_one_type(Archetype& src_archetype);

  template <class Ty>
  Archetype& _del_one_type(Archetype& src_archetype);

 private:
  Id _next_entity_id{0};

  Map<Id, Entity> _entities;

  Map<Id, Archetype> _archetypes;

  //  archetype_id, component_id, archetype_ref
  Map<Id, Map<Id, Archetype&>> archetype_graph;

 public:
  template <class... Tys>
  static Tag tag(Tag const& exist_tag);

  template <class Ty>
  static Id get_id();

 private:
  inline static Id _next_type_idx = 0;

  template <class Ty>
  inline static Id _type_idx_val = _next_type_idx++;
};

#pragma region ECS_ENTITY_IMPL

inline Entity::Entity(Id eid, World* world) : eid(eid), world(world) {}

template <class... Tys>
Entity& Entity::set(Tys&&... args) {
  world->set_components(eid, std::forward<Tys>(args)...);
  return *this;
}

template <class... Tys>
Entity& Entity::add(Tys&&... args) {
  world->add_components(eid, std::forward<Tys>(args)...);
  return *this;
}

template <class... Tys>
std::tuple<Tys&...> Entity::get() {
  return world->get_components<Tys...>(eid);
}

template <class... Tys>
Entity& Entity::del() {
  world->del_components<Tys...>(eid);
  return *this;
}

#pragma endregion

#pragma region ECS_WORLD_IMPL

template <class... Tys>
inline Tag World::tag(Tag const& exist_tag) {
  Tag&& new_tag = {get_id<Tys>()...};
  new_tag.insert(new_tag.end(), exist_tag.begin(), exist_tag.end());
  std::sort(new_tag.begin(), new_tag.end());
  new_tag.erase(std::unique(new_tag.begin(), new_tag.end()), new_tag.end());
  return new_tag;
}

template <class Ty>
inline Id World::get_id() {
  return _type_idx_val<Ty>;
}

template <class... Tys>
inline Query<Tys...> World::query() {
  return Query<Tys...>(this);
}

template <class... Tys>
void World::set_components(Id eid, Tys&&... args) {
  auto& archetype = entity_records.at(eid);
  if (archetype.has<Tys...>()) {
    archetype.set<Tys...>(eid, std::forward<Tys>(args)...);
  } else {
    add_components(eid, std::forward<Tys>(args)...);
  }
}

template <class... Tys>
std::tuple<Tys&...> World::get_components(Id eid) {
  auto& archetype = entity_records.at(eid);
  return archetype.at<Tys...>(eid);
}

template <class... Tys>
void World::add_components(Id eid, Tys&&... args) {
  Archetype& old_table = entity_records.at(eid);
  Archetype& new_table = add_to_archetype<Tys...>(old_table);

  if (&old_table != &new_table) {
    _move_entity(eid, old_table, new_table);
    entity_records.erase(eid);
    entity_records.emplace(eid, new_table);
  }
  new_table.set(eid, std::forward<Tys>(args)...);
}

template <class... Tys>
void World::del_components(Id eid) {
  Archetype& old_table = entity_records.at(eid);
  Archetype& new_table = del_from_archetype<Tys...>(old_table);

  if (&old_table != &new_table) {
    _move_entity(eid, old_table, new_table);
    entity_records.erase(eid);
    entity_records.emplace(eid, new_table);
  }
}

template <class... Tys>
Archetype& World::add_to_archetype(Archetype const& src_archetype) {
  return deduce_archetype</* is_add_or_del = */ true, Tys...>(src_archetype);
}

template <class... Tys>
Archetype& World::del_from_archetype(Archetype const& src_archetype) {
  return deduce_archetype</* is_add_or_del = */ false, Tys...>(src_archetype);
}

template <bool is_add_or_del, class... Ty>
Archetype& World::deduce_archetype(Archetype const& src_archetype) {
  auto&& new_tag = tag<Ty...>(src_archetype.unsort_tag());
  auto table_iter = tag_records.find(new_tag);

  if (table_iter == tag_records.end()) {
    auto tid = _next_entity_id++;
    auto emplace_iter = _archetypes.emplace(tid, Table::creator<is_add_or_del, Ty...>(tid, src_archetype));

    assert(emplace_iter.second && "emplace new archetype failed.");
    auto& new_archetype = emplace_iter.first->second;
    tag_records.emplace(new_tag, new_archetype);
    return new_archetype;
  }

  return table_iter->second;
}

template <class... Ty>
inline Entity& World::entity() {
  static_assert((... && std::is_default_constructible_v<Ty>));

  auto eid = _next_entity_id++;
  Entity new_entity(eid, this);

  auto& dst_archetype = deduce_archetype<true, Ty...>({});
  dst_archetype.add(eid);

  entity_records.emplace(eid, dst_archetype);
  auto p = _entities.emplace(eid, std::move(new_entity));

  return p.first->second;
}

template <class... Ty>
inline Entity& World::entity(Ty&&... args) {
  auto eid = _next_entity_id++;
  Entity new_entity(eid, this);

  auto& dst_archetype = deduce_archetype<true, Ty...>({});
  dst_archetype.add(eid, std::forward<Ty>(args)...);

  entity_records.emplace(eid, dst_archetype);
  auto p = _entities.emplace(eid, std::move(new_entity));

  return p.first->second;
}

void inline World::_move_entity(Id eid, Archetype& src_archetype, Archetype& dst_archetype) { src_archetype.move(eid, dst_archetype); }

#pragma endregion

#pragma region ECS_TABLE_IMPL

void inline Table::_move_elemt(Table& src_table, size_t src_row, size_t src_col, Table& dst_table, size_t dst_row, size_t dst_col) {
  src_table.data[src_row]->move(src_col, *dst_table.data[dst_row], dst_col);
}

void inline Table::move(Id id, Table& dst_table) {
  if (this == &dst_table) return;

  size_t src_col = id2col[id];
  size_t dst_col = 0;

  if (!dst_table.id2col.contains(id)) {
    dst_col = dst_table.cols();
    dst_table.id2col.emplace(id, dst_table.cols());
    dst_table.col2id.emplace_back(id);
  } else {
    dst_col = dst_table.id2col[id];
  }

  for (size_t src_row = 0; src_row < rows(); ++src_row) {
    auto id_row = row2id[src_row];
    auto p = dst_table.id2row.find(id_row);
    if (p != dst_table.id2row.end()) {
      Table::_move_elemt(*this, src_row, src_col, dst_table, p->second, dst_col);
    }
  }
  // delete moved element
  del(id);
}

inline bool Table::ready() const { return data.size() == rows(); }

inline data::IStorage& Table::row(size_t idx) const {
  // TODO: insert return statement here
  return *data[idx];
}

template <class Ty>
data::Array<Ty>& Table::row() const {
  return typed_row<Ty>();
}

inline bool Table::has(Id id) const { return id2col.contains(id); }

template <class... Ty>
bool Table::has() const {
  return (id2row.contains(World::get_id<Ty>()) && ...);
}

void inline Table::del(Id id) {
  // swap_end & delete implementation
  auto col = id2col[id];  // throw exception if not found
  auto last_col = cols() - 1;
  auto last_id = col2id[last_col];

  for (size_t i = 0; i < rows(); ++i) {
    data[i]->swap(col, cols() - 1);
    data[i]->shrink_one();
  }
  std::swap(id2col[id], id2col[last_id]);
  std::swap(col2id[col], col2id[last_col]);
  id2col.erase(id);
  col2id.pop_back();
}

template <class... Ty>
void Table::add(Id id, Ty&&... val) {
  assert(!id2col.contains(id));
  id2col.emplace(id, cols());
  col2id.emplace_back(id);
  (typed_row<Ty>().append(val), ...);
  _append_empty_if_needed();
}

template <class... Ty>
void Table::set(Id id, Ty&&... val) {
  auto col_iter = id2col.find(id);
  if (col_iter == id2col.end()) {
    return add(id, std::forward<Ty>(val)...);
  }
  (([this, &col_iter]<class T>(T&& val) {
     auto& col = col_iter->second;
     auto& data_row = typed_row<Ty>();
     if (data_row.size() > col) {
       data_row.at(col) = std::forward<Ty>(val);
     } else {
       assert(data_row.size() == col);
       // Here the append is needed to add this component, because this entity is partially
       // initialized by outside moving.
       data_row.append(std::forward<Ty>(val));
     }
   }(std::forward<Ty>(val))),
   ...);
}

template <class... Ty>
std::tuple<Ty&...> Table::at(Id id) {
  auto col = id2col.at(id);
  return std::forward_as_tuple(typed_row<Ty>().at(col)...);
}

void inline Table::_append_empty_if_needed() {
  for (auto& row : data) {
    while (row->size() < cols()) {
      row->extend_one();
    }
  }
}

template <class Ty>
data::Array<Ty>& Table::typed_row() const {
  return _typed_row<Ty>(id2row.at(World::get_id<Ty>()));
}

template <class Ty>
data::Array<Ty>& Table::_typed_row(size_t row) const {
  return *utils::down_cast<data::Array<Ty>>(data[row]);
}

template <class... Ty>
inline void Table::add_rows() {
  ((!has<Ty>() && (_add_row<Ty>(), true)), ...);
}

template <class Ty>
inline void Table::_add_row() {
  auto id = World::get_id<Ty>();
  data.emplace_back(new data::Array<Ty>());
  id2row.emplace(id, rows());
  row2id.emplace_back(id);
}

template <class... Ty>
inline void Table::del_rows() {
  ((has<Ty>() && (_del_row<Ty>(), true)), ...);
}

// Please make sure that 'Type' exists in table
template <class Ty>
inline void Table::_del_row() {
  auto id = World::get_id<Ty>();
  auto row = id2row[id];
  auto last_row = rows() - 1;
  auto last_id = row2id[last_row];

  id2row[last_id] = row;
  id2row.erase(id);

  std::swap(data[row], data[last_row]);
  delete data.back();
  data.pop_back();

  std::swap(row2id[row], row2id[last_row]);
  row2id.pop_back();
}

inline Table::Table(Id tid, World* world) : tid(tid), world(world) {}

template <class... Ty>
inline Table Table::creator(Id tid, World* world) {
  auto new_table = Table(tid, world);
  new_table.add_rows<Ty...>();
  return new_table;
}

template <bool is_add_or_del, class... Ty>
inline Table Table::creator(Id tid, Table const& src_table) {
  auto new_table = Table(tid, src_table.world);
  new_table.mimic_rows(src_table);
  if constexpr (is_add_or_del) {
    new_table.add_rows<Ty...>();
  } else {
    new_table.del_rows<Ty...>();
  }
  return new_table;
}

inline Table::~Table() {
  for (auto ptr : data) {
    delete ptr;
  }
}

void inline Table::mimic_rows(Table const& src_table) {
  if (!data.empty()) return;

  row2id = src_table.row2id;
  id2row = src_table.id2row;

  for (size_t ri = 0; ri < src_table.rows(); ++ri) {
    data.emplace_back(src_table.row(ri).create_mimic());
  }
}

#ifndef NEO_VARIENT_TEST

template <class... Ty, class TFn>
inline void Table::for_each_col(TFn&& func) {
  auto&& data_rows = std::forward_as_tuple(typed_row<Ty>()...);
  for (size_t col = 0; col < cols(); ++col) {
    std::apply([col, func](auto&&... data_row) { func(data_row[col]...); }, data_rows);
  }
}

#else

template <class... Ty, class TFn>
inline void Table::for_each_col(TFn&& func) {
  for (size_t col = 0; col < cols(); ++col) {
    func(*static_cast<Ty*>(row(id2col[world->get_id<Ty>()])[col])...);
  }
}

#endif

template <class Ty>
inline std::vector<Ty>& Table::get_row() {
  return this->typed_row<Ty>().inner_array;
}

#pragma endregion

#pragma region Type_Array_Impl

#pragma endregion

#pragma region Query_Impl

template <class... Ty>
Query<Ty...>::Query(World* world) : world(world) {
  initialize();
}

template <class... Ty>
void Query<Ty...>::initialize() {
  const auto tag = world->tag<Ty...>({});
  for (auto& [arch_tag, archetype] : world->tag_records) {
    if (std::includes(arch_tag.begin(), arch_tag.end(), tag.begin(), tag.end())) {
      views.emplace_back(archetype);
    }
  }
}

template <class... Ty>
template <class TFunc>
void Query<Ty...>::for_each(TFunc&& func) {
  for (auto& view : views) {
    view.for_each(std::forward<TFunc>(func));
  }
}

#pragma endregion

}  // namespace ecs
}  // namespace sim
