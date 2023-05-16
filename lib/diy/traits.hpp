#include <utility>
#include <type_traits>

namespace sim::ecs {

struct Traits {
  typedef void (*copy_ctor_t)(void*, const void*);
  typedef void (*move_ctor_t)(void*, void*);
  typedef void (*dtor_t)(void*);

  copy_ctor_t copy_ctor;
  move_ctor_t move_ctor;
  copy_ctor_t copy_assign;
  move_ctor_t move_assign;
  move_ctor_t swap;
  dtor_t dtor;
  std::size_t size;
};

template<class T> Traits create_traits() {
    return {
        .copy_ctor = [](void* dst, const void* src){
          new(dst) T(*(const T*)src);            
        },
        .move_ctor = [](void* dst, void* src) {
          new(dst) T(std::move(*(T*)src));
        },
        .copy_assign = [](void* dst, const void* src) {
          T* t_dst = reinterpret_cast<T*>(dst);
          const T* t_src = reinterpret_cast<const T*>(src);
          (*t_dst) = (*t_src);
         },
        .move_assign = [](void* dst, void* src) {
          T* t_dst = reinterpret_cast<T*>(dst);
          T* t_src = reinterpret_cast<T*>(src);
          (*t_dst) = std::move(*t_src);
        },
        .dtor = [](void* dst) {
          ((T*)dst)->~T();
        },
        .size = sizeof(T)
    };
}

} // namespace ecs