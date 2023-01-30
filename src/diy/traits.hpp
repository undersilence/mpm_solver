#include <utility>

namespace sim::ecs {

struct Traits {
  typedef void (*copy_ctor_t)(void*, const void*);
  typedef void (*move_ctor_t)(void*, void*);
  typedef void (*dtor_t)(void*);

  copy_ctor_t copy_ctor;
  move_ctor_t move_ctor;
  move_ctor_t swap;
  dtor_t dtor;
  size_t size;
};

template<class T> Traits create_traits() {
    return {
        .copy_ctor = [](void* dst, const void* src){
          new(dst) T(*(const T*)src);            
        },
        .move_ctor = [](void* dst, void* src) {
          new(dst) T(std::move(*(T*)src));
        },
        .swap = [](void* lhs, void* rhs) {
          T* tl = (T*)lhs;
          T* tr = (T*)rhs;
          T temp = std::move(*tl);
          (*tl) = std::move(T(*tr));
          (*tr) = std::move(temp);
        },
        .dtor = [](void* dst) {
          ((T*)dst)->~T();
        },
        .size = sizeof(T)
    };
}

} // namespace ecs