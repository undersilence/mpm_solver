// C-like type erasure techniques.
#pragma once
#include <algorithm>
#include <cstddef>
#include <new>

namespace sim::ecs {
// c-style type erasure but implement it with cpp-style ;)
// pity that the move semantic is not supported by this implementation, and idk how to.
// try to implement emplace_back but failed with the template dependency of Traits
struct vec_core_t {
public:
  typedef void* value_type;
  typedef const void* const_value_type;

  typedef void* iterator;
  typedef const void* const_iterator;

public:
  struct Traits {
    typedef void (*Ctor)(value_type, const_value_type);
    typedef void (*Dtor)(value_type);
    Ctor ctor;
    Dtor dtor;
    size_t size;
    Traits(Ctor ctor, Dtor dtor, size_t size) : ctor(ctor), dtor(dtor), size(size) {}
  };

public:
  template <class T> struct Element {
    static void ctor(value_type to, const_value_type from) {
      new (to) Element(std::move(*static_cast<Element const*>(from)));
    }
    static void dtor(value_type p) { static_cast<Element*>(p)->Element ::~Element(); }
    T client;
  };

public:
  explicit vec_core_t(Traits const& traits);
  vec_core_t(vec_core_t&&) = default;
  vec_core_t(const vec_core_t&) = default;
  ~vec_core_t();

  value_type operator[](size_t) const;
  size_t size() const;
  bool empty() const;
  void insert(const_iterator where, const_iterator src_begin, const_iterator src_end) const;
  iterator begin();
  iterator end();
  iterator begin() const;
  iterator end() const;
  iterator advance(iterator iter, size_t step);
  void resize(size_t) const;
  bool reserve(size_t) const;
  void append(value_type) const;

public:
  struct Impl;
  Impl* pimpl {};
};

template <class T> vec_core_t create_vec() {
  return vec_core_t(
      vec_core_t::Traits(vec_core_t::Element<T>::ctor, vec_core_t::Element<T>::dtor, sizeof(T)));
}
} // namespace sim::ecs