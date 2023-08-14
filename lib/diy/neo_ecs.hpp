#pragma once

#include "forward.hpp"
#include <c++/v1/__utility/integer_sequence.h>
#include <cassert>
#include <algorithm>
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
  return FUNCTION_SIG;
}

template<template<class> class TVec, class Ty>
bool is_subset(TVec<Ty>&& S, TVec<Ty>&& I) {
  auto p = std::begin(S);
  auto q = std::begin(I);
  while(p != std::end(S) && q != std::end(I)) {
    if(*p < *q) {
      ++p;
    } 
    else if(*p == *q) {
      ++p;
      ++q;
    }
    else {
      break;
    }
  }

  return q ==std::end(I);
}

}


namespace data {

struct IStorage {
public:
  virtual ~IStorage() {}
  virtual void extend_one() {};
  virtual void shrink_one() {};

public:
  // NOTE: use mimic() to create same_type empty instance
  virtual IStorage* create_mimic() = 0;
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

  IStorage* create_mimic() override {
    return new Array<Ty>();
  }

  void swap(size_t i, size_t j) override {
    std::swap(inner_array.at(i), inner_array.at(j));
  }

  void move(size_t src_idx, IStorage& dst_array, size_t dst_idx) override {
    auto& typed_array = static_cast<Array<Ty>&>(dst_array);
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
      append(std::move(Ty()));
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
  Entity& add(Tys&&... args);

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

  template<class... Ty>
  struct _iterator {
    using value_type = std::tuple<Ty...>;
    using reference_type = std::tuple<Ty&...>;
    using difference_type = ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    Table* self;
    size_t col_idx;
    
    _iterator() = default;

    _iterator(Table* self, size_t idx){
      set(self, idx);
    } 

    void set(Table* self, size_t idx) {
      this->self = self;
      this->col_idx = idx;

    }   

    reference_type operator *() {
      return std::forward_as_tuple(
        self->_typed_row<Ty>().at(col_idx)...
      );
    } 

    _iterator& advance(ptrdiff_t t) {
      col_idx += t;
    }
  };

  template<class... Ty>
  _iterator<Ty...> begin();

  template<class... Ty>
  _iterator<Ty...> end();

private:
  std::vector<Id> col2id;
  std::vector<Id> row2id;
  std::unordered_map<Id, size_t> id2col;
  std::unordered_map<Id, size_t> id2row;
  std::vector<data::IStorage*> data;

public:
  Table(Id tid, World* world);
  ~Table();
  Table(Table&& table) = default;
  Table(Table const&) = delete;
  Table& operator = (Table&& table) = default;
  Table& operator = (Table const& table) = delete;

  template<class... Ty> 
  void add_rows();

  template<class... Ty>
  void del_rows();

  // use this function to create mimic empty row instance
  void mimic_rows(Table& other_table);

  size_t rows() const { return row2id.size(); }
  size_t cols() const { return col2id.size(); }
  bool ready() const;
  
  data::IStorage& row(size_t idx);

  // Helper: get row by type
  template<class Ty>
  data::Array<Ty>& row();

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
  
  template<class... Ty, class TFn>
  void for_each_col(TFn&& func);

private:

  void _append_empty_if_need();

  static void _move_elemt(
    Table& src_table, size_t src_row, size_t src_col,
    Table& dst_table, size_t dst_row, size_t dst_col);

  template<class Ty>
  data::Array<Ty>& _typed_row();

  template<class Ty>
  data::Array<Ty>& _typed_row(size_t row);

  template<class Ty>
  void _del_row();

  template<class Ty>
  void _add_row();

  void _del_col(Id id);
  
  void _add_col(Id id);
};
using Archetype = Table;


template<class... Ty>
struct Query {
public:
  
  Query(struct World*);

  struct iterator {
    using _TyIt = Archetype::_iterator<Ty...>; 
    using value_type = _TyIt::value_type;
    using reference_type = _TyIt::reference_type;
    using iterator_category = _TyIt::iterator_category;
    using difference_type = _TyIt::difference_type;
    
    Query* self;
    _TyIt it;

    iterator() = default;
    iterator(Query* self, _TyIt it): self(self), it(it) {}

    iterator& advance(difference_type step_size) {
      // move forward
      while(step_size + it.col_idx > it.self->cols()) {
        
      }
      // move backward
      
    }

  };
  
  void initialize();

  iterator::value_type at(size_t idx);
  
  iterator begin();
  iterator end();

  template<class TFn>
  void for_each(TFn&& func);

private:
  struct World* world;
  std::vector<Archetype*> archetypes;
};

struct World {
public:

  template<class... Ty>
  using Map = std::unordered_map<Ty...>;

  Entity& entity(); 

  Archetype& archetype(Tag&& tag);

  template<class... Ty>
  Query<Ty...> query();

  template<class... Tys>
  std::vector<Id> tag();

  template<class Ty>
  Id get_id();
  
  template<class... Tys>
  void set_components(Id eid, Tys&... value);

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

public: 
  Map<Id, Archetype&> entity_records;
  Map<Tag, Archetype&, utils::vec_hash_t<Tag>, utils::vec_equal_t<Tag>> tag_records;

private:
  void _move_entity(Id eid, Archetype& src_archetype, Archetype& dst_archetype);
  
  template<class Ty>
  Archetype& _add_one_type(Archetype& src_archetype);

  template<class Ty>
  Archetype& _del_one_type(Archetype& src_archetype);

private:
  Id next_id {0};
  Map<Id, Entity> entities_;
  Map<Id, Archetype> archetypes_;
  Map<const char*, Id> _type_index;

  //  archetype_id, component_id, archetype_ref
  Map<Id, Map<Id, Archetype&>> archetype_graph;
};


#pragma region ECS_ENTITY_IMPL

inline Entity::Entity(Id eid, World* world) : eid(eid), world(world) {}

template<class... Tys>
Entity& Entity::set(Tys&&... args){
  world->add_components(eid, std::forward<Tys>(args)...);
  return *this;
}

template<class... Tys>
Entity& Entity::add(Tys&&... args) {
  return set(std::forward<Tys>(args)...);
}

template<class... Tys>
std::tuple<Tys&...> Entity::get() {
  return world->get_components<Tys...>(eid);
}


#pragma endregion

#pragma region ECS_WORLD_IMPL

template<class... Tys>
std::vector<Id> World::tag() {
  auto tag = Tag{get_id<Tys>()...};
  std::sort(tag.begin(), tag.end());
  tag.erase(std::unique(tag.begin(), tag.end()), tag.end());
  return tag;
}


template<class Ty>
Id World::get_id() {
  auto type_iter = _type_index.find(utils::id<Ty>());
  if(type_iter == _type_index.end()) {
    _type_index.emplace(utils::id<Ty>(), next_id);
    return next_id++;
  }
  return type_iter->second;
}

template <class... Tys> 
inline void World::set_components(Id eid, Tys&... args) {
  auto& archetype = entity_records.at(eid);
  archetype.set<Tys...>(eid, std::forward<Tys>(args)...);
}

template<class... Tys>
std::tuple<Tys&...> World::get_components(Id eid) {
  auto& archetype = entity_records.at(eid);
  return archetype.at<Tys...>(eid);
}


template<class... Tys>
void World::add_components(Id eid, Tys&&... args) {
  Archetype& old_table = entity_records.at(eid);
  Archetype& new_table = add_to_archetype<Tys...>(old_table);

  _move_entity(eid, old_table, new_table);
  new_table.add(eid, std::forward<Tys>(args)...);

  entity_records.erase(eid);
  entity_records.emplace(eid, new_table);
}

template<class... Tys>
void World::del_components(Id eid) {
  Archetype& src_archetype = entity_records.at(eid);
  Archetype& dst_archetype = del_from_archetype<Tys...>(src_archetype);
  
  _move_entity(eid, src_archetype, dst_archetype);

  entity_records.erase(eid);
  entity_records.emplace(eid, dst_archetype);
}


template<class... Tys>
Archetype& World::add_to_archetype(Archetype& src_archetype) {
  auto cur_tag = src_archetype.tag();
  auto&& new_tag = tag<Tys...>();
  cur_tag.insert(cur_tag.end(), 
    std::make_move_iterator(new_tag.begin()),
    std::make_move_iterator(new_tag.end()));

  auto& dst_archetype = archetype(std::move(cur_tag));
  if(!dst_archetype.ready()) {
    dst_archetype.mimic_rows(src_archetype);
    dst_archetype.add_rows<Tys...>();
    assert(dst_archetype.ready());
  }
  return dst_archetype;
}


template<class... Tys>
Archetype& World::del_from_archetype(Archetype &src_archetype) {
  auto cur_tag = src_archetype.tag();
  (cur_tag.erase(std::remove(cur_tag.begin(), cur_tag.end(), get_id<Tys>()), cur_tag.end()), ...);

  auto& dst_archetype = archetype(std::move(cur_tag));
  if(!dst_archetype.ready()) {
    dst_archetype.mimic_rows(src_archetype);
    dst_archetype.del_rows<Tys...>();
    assert(dst_archetype.ready());
  }
  return dst_archetype;
}


inline Archetype& World::archetype(Tag&& tag) {
  std::sort(tag.begin(), tag.end());
  auto arche_iter = tag_records.find(tag);
  if(arche_iter == tag_records.end()) {
    auto aid = next_id++;
    Archetype new_archetype(aid, this);
    // NOTE: new_archetype initialized here
    auto p = archetypes_.emplace(aid, std::move(new_archetype));

    assert(p.second && "emplace new archetype failed!");
    tag_records.emplace(tag, p.first->second);
    return p.first->second;
  }
  return arche_iter->second;
}


inline Entity& World::entity() {
  auto eid = next_id++;
  Entity new_entity(eid, this);
  auto& empty_archetype = archetype({});
  empty_archetype.add(eid);
  
  entity_records.emplace(eid, empty_archetype);
  auto p = entities_.emplace(eid, std::move(new_entity));

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
  size_t dst_col = 0;

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


inline bool Table::ready() const { 
  return data.size() == rows(); 
}


inline data::IStorage& Table::row(size_t idx) {
  // TODO: insert return statement here
  return *data[idx];
}


template<class Ty>
data::Array<Ty>& Table::row() {
  return _typed_row<Ty>();
}


inline bool Table::has(Id id) {
  return id2col.contains(id);
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
  auto col_iter = id2col.find(id);
  if (col_iter == id2col.end()) {
    return add(id, std::forward<Ty>(val)...);
  }
  ((_typed_row<Ty>().at(col_iter->second) = val), ...);
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
  return _typed_row<Ty>(id2row.at(world->get_id<Ty>()));
}


template<class Ty>
data::Array<Ty>& Table::_typed_row(size_t row) {
  return *utils::cast<data::Array<Ty>>(data.at(row));
}


template <class... Ty> void Table::add_rows() {
  (data.emplace_back(new data::Array<Ty>()), ...);
  ((id2row.emplace(world->get_id<Ty>(), rows()), 
    row2id.emplace_back(world->get_id<Ty>())), ...);
}


template <class... Ty> inline void Table::del_rows() {
  if constexpr (sizeof...(Ty) > 1) {
    (del_row<Ty>(), ...); 
  } else {
    (_del_row<Ty>(), ...);
  }
}


template <class Ty> inline void Table::_del_row() {
  auto id = get_id<Ty>();
  auto row = id2row.at(id);
  auto last_row = rows() - 1;
  auto last_id = row2id.at(last_row);
  
  std::swap(data.at(row), data.at(last_row));
  std::swap(row2id.at(row), row2id.at(last_row));
  
  delete data.back();
  data.pop_back();
  id2row.erase(last_id);
  row2id.pop_back();
}


inline Table::Table(Id tid, World* world) : tid(tid), world(world) {}


inline Table::~Table() {
  for(auto ptr : data) {
    delete ptr;
  }
}


void inline Table::mimic_rows(Table& src_table) {
  row2id = src_table.row2id;
  id2row = src_table.id2row;
  for(size_t ri = 0; ri < src_table.rows(); ++ri) {
    data.emplace_back(src_table.row(ri).create_mimic());
  }
}


template<class... Ty, class TFn>
inline void Table::for_each_col(TFn&& func) {
  for(size_t col = 0; col < cols(); ++col) {
    func(_typed_row<Ty>().at(col)...);
  }
}

#pragma endregion

#pragma region Type_Array_Impl


#pragma endregion

#pragma region Query_Impl

template<class... Ty>
Query<Ty...>::Query(World* world): world(world) {}

template<class... Ty>
void Query<Ty...>::initialize() {
  auto cur_tag = world->tag<Ty...>();
  for(auto& [tag, archetype] : world->tag_records) {
    if(utils::is_subset(tag, cur_tag)) {
      archetypes.emplace_back(&archetype);
    }
  }
}

template<class... Ty>
template<class TFn>
void Query<Ty...>::for_each(TFn&& func) {
  auto cur_tag = world->tag<Ty...>();
  for(auto& [tag, archetype] : world->tag_records) {
    if(utils::is_subset(tag, cur_tag)) {
      archetype.for_each_col<Ty...>(std::forward<TFn>(func));
    }
  }
}

template<class... Ty>
Query<Ty...>::iterator Query<Ty...>::begin() {
  return {this, Archetype::_iterator<Ty...>(archetypes.empty() ? nullptr : archetypes.front(), 0)}; 
}

template<class... Ty>
Query<Ty...>::iterator Query<Ty...>::end() {
  return {this, Archetype::_iterator<Ty...>(archetypes.empty() ? nullptr : archetypes.back(), archetypes.back()->cols())};
}

#pragma endregion

}}