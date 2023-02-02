// C-like type erasure techniques.
#pragma once
#include <algorithm>
#include <cstddef>
#include <new>

#include "diy/traits.hpp"

namespace sim::ecs {
// c-style type erasure but implement it with cpp-style ;)
// pity that the move semantic is not supported by this implementation, and idk how to.
// try to implement emplace_back but failed with the template dependency of Traits
struct vec_core_t {
public:
  typedef void* value_type;
  typedef const void* const_value_type;
//
  typedef void* iterator;
  typedef const void* const_iterator;
public:
  // struct Traits {
  //   typedef void (*Ctor)(value_type, const_value_type);
  //   typedef void (*Dtor)(value_type);
  //   // typedef void (*Def_Ctor)(value_type);
  //   Ctor ctor;
  //   Dtor dtor;
  //   // Def_Ctor def_ctor;
  //   size_t size;
  //   Traits(Ctor ctor, Dtor dtor, size_t size) : ctor(ctor), dtor(dtor), size(size) {}
  // };

public:
  // template <class T> struct Element {
  //   static void ctor(value_type to, const_value_type from) {
  //     new (to) Element(*static_cast<Element const*>(from));
  //   }
  //   static void def_ctor(value_type to) {
  //     new (to) Element();
  //   }
  //   static void dtor(value_type p) { static_cast<Element*>(p)->Element ::~Element(); }
  //   T client;
  // };

public:
  explicit vec_core_t(Traits const& traits);
  vec_core_t(vec_core_t&&) noexcept ;
  vec_core_t(const vec_core_t&);
  ~vec_core_t();
  value_type operator[](size_t);
  value_type operator[](size_t) const;
  size_t size() const;
  bool empty() const;
  void insert(const_iterator where, const_iterator src_begin, const_iterator src_end);
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  iterator advance(iterator iter, size_t step) const;
  void resize(size_t) const;
  bool reserve(size_t) const;
  void push_back(value_type) const;
  void pop_back() const; // pop_back
  void clear() const;
  void swap(size_t i, size_t j) const;
  void* data() const;

public:
  struct Impl;
  Impl* pimpl {};
};

template <class T> vec_core_t create_vec() {
  return vec_core_t(create_traits<T>());
}
} // namespace sim::ecs