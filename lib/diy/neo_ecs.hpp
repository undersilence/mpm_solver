#pragma once

#include "diy/ecs.hpp"
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>
#include <utility>
#include <unordered_map>

namespace sim::neo {

namespace utils {

template<class TDerived, class TBase>
auto cast(TBase* ptr) -> TDerived* {
  return static_cast<TDerived*>(ptr);
}

template<class TVec>
struct vec_hash_t {
  size_t operator()(const TVec& v) const {
    size_t hash = v.size();
    for (auto& x : v)
      hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

template<class TVec>
struct vec_equal_t {
  bool operator()(const TVec& lhs, const TVec& rhs) const { return lhs == rhs; }
};

template<class Ty>
constexpr static inline const char* id() {
  return __PRETTY_FUNCTION__;
}

}


namespace data {

struct IStorage {
public:
  virtual ~IStorage() {}
  virtual void extend_one() {};
  virtual void shrink_one() {};

public:
  virtual size_t size() const = 0;
  virtual void swap(size_t i, size_t j) = 0;
  virtual void * operator[] (size_t offset) = 0;
  virtual void move(size_t src_idx, IStorage& dst, size_t dst_idx) = 0;
};


template<typename Ty>
struct Array : IStorage {
private:
  std::vector<Ty> inner_array;

public:

  void swap(size_t i, size_t j) override {
    std::swap(inner_array.at(i), inner_array.at(j));
  }

  void move(size_t src_idx, IStorage& dst_array, size_t dst_idx) override {
    auto& typed_array = static_cast<Array<Ty>&>(dst_array);
    if(dst_idx > dst_array.size()) {
      
    }
    typed_array.insert(dst_idx, std::move(inner_array.at(src_idx)));
  }

  template<class _Ty>
  auto append(_Ty&& val) {
    return inner_array.emplace_back(val);
  }

  Ty& at(size_t i) {
    return inner_array.at(i);
  }

  size_t size() const override {
    return inner_array.size();
  }

  void* operator[] (size_t idx) override {
    return &at(idx);
  }

  void extend_one() override {
    if constexpr (std::is_default_constructible_v<Ty>) {
      append({});
    } else {
      assert(false && "value must be set properly for non_default_constructible class");
    }
  }

  void shrink_one() override {
    inner_array.pop_back();
  }

  void insert(size_t pos, Ty&& val) {
    inner_array.insert(inner_array.begin() + pos, val);
  }
};
}

namespace ecs {

using Id = uint32_t;
using Tag = std::vector<Id>;

struct Entity {
public:
  template<typename... Tys>
  Entity& set(Tys&&... args);

  template<typename... Tys>
  bool has();

  template<typename... Tys>
  std::tuple<Tys&...> get();

  template<typename... Tys>
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
  Table(Id tid, World* world): tid(tid), world(world) {}
  Table(Table&& table) = default;
  Table(Table const&) = delete;
  Table& operator = (Table&& table) = default;
  Table& operator = (Table const& table) = delete;

  template<class... Ty> 
  void initialize_rows();

  template<class... Ty>
  void mimic(Table& other_table);

  ~Table() {
    for(auto ptr : data) {
      delete ptr;
    }
  }

  size_t rows() const { return row2id.size(); }
  size_t cols() const { return col2id.size(); }
  std::vector<Id> tag() const { return row2id; };

  template<class... Ty>
  std::tuple<Ty&...> at(Id id);

  template<class... Ty>
  void set(Id id, Ty&&... val);

  bool has(Id id);

  template<class... Ty>
  void add(Id id, Ty&&... val);

  void del(Id id);

  void move(Id id, Table& dst_table);

private:

  void _append_empty_if_need();

  static void _move_elemt(
    Table& src_table, size_t src_row, size_t src_col,
    Table& dst_table, size_t dst_row, size_t dst_col);

  template<class Ty>
  data::Array<Ty>& _typed_row();
};


struct World {
public:

  typedef Table Archetype;

  Entity& entity(); 
  Archetype& archetype(Tag&& tag);

  template<class... Tys>
  std::vector<Id> tag();

  template<class Ty>
  Id get_id();
  
  template<class... Tys>
  void add_components(Id eid, Tys&&... value);

  template<class... Tys>
  void del_components(Id eid);

  template<class... Tys>
  void has_components(Id eid);

  template<class... Tys>
  std::tuple<Tys&...> get_components(Id eid);

public:
  template<class... Tys>
  Archetype& add_to_archetype(Archetype& src);

  template<class... Tys>
  Archetype& del_from_archetype(Archetype& src);

private: 
  Id next_id {0};
  std::unordered_map<const char*, Id> type_index;
  std::unordered_map<Id, Archetype&> entity_records;
  std::unordered_map<Tag, 
    Archetype&, 
    utils::vec_hash_t<Tag>, 
    utils::vec_equal_t<Tag>> tag_records;

private:
  void _move_entity(Id eid, Archetype& src_archetype, Archetype& dst_archetype);

private:
  std::unordered_map<Id, Entity> entities_;
  std::unordered_map<Id, Archetype> archetypes_;
};


#pragma region ECS_ENTITY_IMPL

inline Entity::Entity(Id eid, World* world) : eid(eid), world(world) {}

template<class... Tys>
Entity& Entity::set(Tys&&... args){
  world->add_components(eid, std::forward<Tys>(args)...);
  return *this;
}

template<class... Tys>
std::tuple<Tys&...> Entity::get() {
  return world->get_components<Tys...>(eid);
}


#pragma endregion

#pragma region ECS_WORLD_IMPL

template<class... Tys>
std::vector<Id> World::tag() {
  Tag tag;
  (tag.emplace_back(get_id<Tys>()), ...);
  std::sort(tag.begin(), tag.end());
  return tag;
}


template<class Ty>
Id World::get_id() {
  auto type_iter = type_index.find(utils::id<Ty>());
  if(type_iter == type_index.end()) {
    type_index.emplace(utils::id<Ty>(), next_id);
    return next_id++;
  }
  return type_iter->second;
}


template<class... Tys>
std::tuple<Tys&...> World::get_components(Id eid) {
  auto& archetype = entity_records.at(eid);
  return archetype.at<Tys...>(eid);
}


template<class... Tys>
void World::add_components(Id eid, Tys&&... args) {
  auto& old_table = entity_records.at(eid);
  auto& new_table = add_to_archetype<Tys...>(old_table);

  _move_entity(eid, old_table, new_table);
  new_table.add(eid, std::forward<Tys>(args)...);
}


template<class... Tys>
World::Archetype& World::add_to_archetype(World::Archetype& src_archetype) {
  auto cur_tag = src_archetype.tag();
  auto&& new_tag = tag<Tys...>();
  cur_tag.insert(cur_tag.end(), 
    std::make_move_iterator(new_tag.begin()),
    std::make_move_iterator(new_tag.end()));
  auto& dst_archetype = archetype(std::move(cur_tag));
  dst_archetype.mimic<Tys...>(src_archetype); // TODO: initialize rows based on src_archetype
  return dst_archetype;
}


template<class... Tys>
World::Archetype& World::del_from_archetype(Archetype &src) {

}


inline World::Archetype& World::archetype(Tag&& tag) {
  std::sort(tag.begin(), tag.end());
  auto arche_iter = tag_records.find(tag);
  if(arche_iter == tag_records.end()) {
    Archetype new_archetype(next_id, this);
    // NOTE: new_archetype initialized here
    auto p = archetypes_.emplace(next_id, std::move(new_archetype));
    next_id++;

    assert(p.second && "emplace new archetype failed!");
    tag_records.emplace(tag, p.first->second);
    return p.first->second;
  }
  return arche_iter->second;
}


inline Entity& World::entity() {
  Entity new_entity(next_id, this);
  auto p = entities_.emplace(next_id, std::move(new_entity));
  auto& empty_archetype = archetype({});
  entity_records.emplace(next_id, empty_archetype);
  next_id++;

  return p.first->second;
}


void inline World::_move_entity(Id eid, Archetype& src_archetype, Archetype& dst_archetype) {
  src_archetype.move(eid, dst_archetype);
}

#pragma endregion


#pragma region ECS_TABLE_IMPL

void inline Table::_move_elemt(
  Table& src_table, size_t src_row, size_t src_col,
  Table& dst_table, size_t dst_row, size_t dst_col) {
  src_table.data[src_row]->move(src_col, *dst_table.data[dst_row], dst_col);
}


void inline Table::move(Id id, Table& dst_table) {
  if(this == &dst_table) return;
  size_t src_col = id2col[id];
  size_t dst_col = -1;

  if(!dst_table.id2col.contains(id)) {
    dst_col = dst_table.cols();
    dst_table.id2col.emplace(id, dst_table.cols());
    dst_table.col2id.emplace_back(id);
  } else {
    dst_col = dst_table.id2col.at(id);
  }

  for(size_t src_row = 0; src_row < row2id.size(); ++src_row) {
    auto id_row = row2id.at(src_row);
    auto dst_row = dst_table.id2row.at(id_row);
    Table::_move_elemt(*this, src_row, src_col, dst_table, dst_row, dst_col);    
  }
  // delete moved element
  del(id);
}


void inline Table::del(Id id) {
  // swap_end & delete implementation
  auto col = id2col.at(id); // throw exception if not found
  auto last_col = cols() - 1;
  auto last_id = col2id.at(last_col);

  for(size_t i = 0; i < rows(); ++i) {
    data[i]->swap(col, cols() - 1);
    data[i]->shrink_one();
  }
  std::swap(id2col.at(id), id2col.at(last_id));
  std::swap(col2id.at(col), col2id.at(last_col));
  id2col.erase(id);
  col2id.pop_back();
}


template <class... Ty> void Table::add(Id id, Ty&&... val) {
  id2col.emplace(id, cols());
  col2id.emplace_back(id);
  (_typed_row<Ty>().append(val), ...);
  _append_empty_if_need();
}


template <class... Ty> void Table::set(Id id, Ty&&... val) {
  if (!id2col.contains(id)) {
    return add(id, std::forward<Ty...>(val...));
  }
  auto col = id2col.at(id);
  ((_typed_row<Ty>().at(col) = val), ...);
}


template <class... Ty> std::tuple<Ty&...> Table::at(Id id) {
  auto col = id2col.at(id);
  return std::forward_as_tuple(
    _typed_row<Ty>().at(col)...
  );
}


void inline Table::_append_empty_if_need() {
  for(auto& row: data) {
    while (row->size() < cols()) {
      row->extend_one();
    }
  }
}

template<class Ty>
data::Array<Ty>& Table::_typed_row() {
  auto row = world->get_id<Ty>();
  return *utils::cast<data::Array<Ty>>(data.at(row));
}

template<class... Ty>
void Table::initialize_rows() {
  (data.emplace_back(new data::Array<Ty>()), ...);
  (row2id.emplace_back(world->get_id<Ty>()), ...);
  for(size_t row = 0; row < row2id.size(); ++row) {
    id2row.emplace(row2id[row], row);
  }
}

template<class... Ty>
void Table::mimic(Table& src_table) {
  for(size_t row = 0; row < src_table.rows(); ++row) {
  }
}

#pragma endregion

#pragma region Type_Array_Impl


#pragma endregion
}

}