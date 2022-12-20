#include <iostream>
#include <vector>
#include <any>

struct Dummy {
  int x;
  Dummy() = default;
  Dummy(Dummy&&) = default;
  Dummy(const Dummy& d) {
    x = d.x;
    static int count = 0;
    printf("copy dummy %d, times: %d\n", x, ++count);
  }
  Dummy(int x) : x(x) {}
};

template<typename... Comps> std::vector<std::any> create_elements (Comps&&... value) {
  return {std::make_any<Comps>(std::forward<Comps>(value))...};
}

template<typename... Dummies> std::vector<Dummy> create_dummies (Dummies&&... value) {
  std::vector<Dummy> dummies {};
  (dummies.emplace_back(std::forward<Dummies>(value)),...);
  return dummies;
}

int main() {
  auto dummies = create_dummies(Dummy(1));
  auto elements = create_elements(Dummy(2));
  std::vector<Dummy> w;
  w.emplace_back(std::move(Dummy(3)));
  return 0;
}