#include "vec.hpp"
#include "mem.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace sim::ecs {

struct vec_core_t::Impl {
public:
  Impl(const Impl& impl)
      : m_traits(impl.m_traits), m_capacity(impl.m_capacity), m_size(impl.m_size),
        m_elements(nullptr) {
    // deep copy
    reserve(m_capacity);
    memcpy(m_elements, impl.m_elements, m_size * m_traits.size);
  }
  explicit Impl(const Traits& traits)
      : m_traits(traits), m_capacity(0), m_size(0), m_elements(nullptr) {
    reserve(3);
  }
  ~Impl() {
    for (size_t i = 0; i < m_size; ++i) {
      m_traits.dtor(address_of(m_elements, i));
    }
    delete[] (char8_t*)m_elements;
  }

public:
  value_type operator[](size_t idx) { return address_of(m_elements, idx); }
  value_type operator[](size_t idx) const { return address_of(m_elements, idx); }

  inline void* address_of(const void* ptr, size_t offset) const {
    return (char8_t*)ptr + offset * m_traits.size;
  }

  static ptrdiff_t byte_diff(const void* ptr_a, const void* ptr_b) {
    return (char8_t*)ptr_a - (char8_t*)ptr_b;
  }

  iterator end() { return address_of(m_elements, m_size); }
  iterator begin() { return m_elements; }
  const_iterator begin() const { return m_elements; }
  const_iterator end() const { return address_of(m_elements, m_size); }

  void move_elements_backwards(const void* first, const void* last, const void* dest) const {
    auto p_first = (char8_t*)first;
    auto p_last = (char8_t*)last;
    auto p_dest = (char8_t*)dest;
    while (p_first != p_last) {
      value_type to = (p_dest -= m_traits.size);
      value_type from = (p_last -= m_traits.size);
      m_traits.move_ctor(to, from);
    }
  }

  void* insert(const void* where, const void* src_begin, const void* src_end) {
    ptrdiff_t insert_bytes = byte_diff(src_end, src_begin);
    auto move_bytes = byte_diff(end(), where);
    auto _offset = byte_diff(where, begin());
    if (m_capacity < insert_bytes / m_traits.size + m_size) {
      // reminder that end() will change when reserve new space
      reserve(std::max(m_capacity + m_capacity, insert_bytes / m_traits.size + m_size + 1));
    }
    auto p_where = (char8_t*)m_elements + _offset;
    auto p_first = (char8_t*)src_begin;
    auto p_last = (char8_t*)src_end;
    if (move_bytes > 0 && _offset >= 0) {
      move_elements_backwards(p_where, end(), (char8_t*)end() + insert_bytes);
    }
    while (p_first != p_last) {
      value_type to = p_where;
      value_type from = p_first;
      m_traits.copy_ctor(to, from);
      //      m_traits.dtor(from);
      p_where += m_traits.size;
      p_first += m_traits.size;
      m_size++;
    }
    return p_where;
  }

  bool reserve(size_t capacity) {
    if (capacity > m_capacity) {
      void* elements = ::operator new(capacity* m_traits.size);
      if (!elements)
        return exit(-1), "Alloc Mem Error.";
      for (size_t i = 0; i < m_size; ++i) {
        value_type to = address_of(elements, i);
        value_type from = address_of(m_elements, i);
        m_traits.move_ctor(to, from);
      }
      if (m_elements != nullptr)
        delete reinterpret_cast<char*>(m_elements);
      m_elements = elements;
      m_capacity = capacity;
      return true;
    }
    return false;
  }

  //  void resize(size_t size) {
  //    if (size > m_size) {
  //      reserve(size);
  //      do {
  //         append()
  //      } while(m_size < size);
  //    }
  //
  //    m_size = size;
  //  }

  // return next element placement address
  void* next_place() {
    if (m_size == m_capacity) {
      reserve(m_capacity + m_capacity);
    }
    return end();
  }

  void append(void* data) {
    if (m_size == m_capacity) {
      reserve(m_capacity + m_capacity);
    }
    m_traits.copy_ctor(end(), data); // copy construct
    m_size++;
  }

  void pop() {
    if (m_size == 0)
      return;
    m_traits.dtor(address_of(m_elements, --m_size));
  }

  void swap(size_t i, size_t j) {
    m_traits.swap(address_of(m_elements, i), address_of(m_elements, j));
  }

public:
  Traits m_traits;
  size_t m_capacity;
  size_t m_size;
  void* m_elements;
};

// very useful
vec_core_t::vec_core_t(vec_core_t&& other) noexcept {
  pimpl = other.pimpl;
  other.pimpl = nullptr;
}
vec_core_t::vec_core_t(const vec_core_t& other) { pimpl = new Impl(*other.pimpl); }
vec_core_t::vec_core_t(const Traits& traits) { pimpl = new Impl(traits); }
vec_core_t::~vec_core_t() { if(pimpl) delete pimpl; }

vec_core_t::value_type vec_core_t::operator[](size_t x) { return pimpl->operator[](x); }
vec_core_t::value_type vec_core_t::operator[](size_t x) const { return pimpl->operator[](x); }
void vec_core_t::insert(vec_core_t::const_iterator where, vec_core_t::const_iterator src_begin,
                        vec_core_t::const_iterator src_end) {
  pimpl->insert(where, src_begin, src_end);
}
vec_core_t::iterator vec_core_t::begin() { return pimpl->begin(); }
vec_core_t::iterator vec_core_t::end() { return pimpl->end(); }
vec_core_t::const_iterator vec_core_t::begin() const { return pimpl->begin(); }
vec_core_t::const_iterator vec_core_t::end() const { return pimpl->end(); }
vec_core_t::iterator vec_core_t::advance(vec_core_t::iterator iter, size_t step) const {
  return (char8_t*)iter + step * pimpl->m_traits.size;
}
size_t vec_core_t::size() const { return pimpl->m_size; }
bool vec_core_t::empty() const { return pimpl->m_size > 0; }
void vec_core_t::push_back(vec_core_t::value_type data) const { return pimpl->append(data); }
void vec_core_t::pop_back() const { return pimpl->pop(); }
// void vec_core_t::resize(size_t size) const {return pimpl->resize(size);}
bool vec_core_t::reserve(size_t capacity) const { return pimpl->reserve(capacity); }
// void vec_core_t::clear() const { return pimpl->clear(); }
void vec_core_t::swap(size_t i, size_t j) const { pimpl->swap(i, j); }

} // namespace sim::ecs