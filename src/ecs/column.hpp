//
// Created by 立 on 2022/12/21.
// C-like type erasure techniques.
#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <functional>

struct Column {
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
    Traits(Ctor ctor, Dtor dtor, size_t size);
  };
public:
  Column(Traits const& traits);
  Column(Column const& column_core);
public:
  struct Impl;
  Impl* pimpl;
};
