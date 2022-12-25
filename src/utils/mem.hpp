#pragma once

namespace sim {
void memswap(void* first, void* last, void* dest) {
  auto p_first = (char8_t*)first;
  auto p_last = (char8_t*)last;
  auto p_dest = (char8_t*)dest;

  while(p_first != p_last) {
    auto temp = *p_first;
    *p_first = *p_dest;
    *p_dest = temp;

    p_first++;
    p_dest++;
  }
}
} // namespace sim