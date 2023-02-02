#pragma once

#include <Eigen/Core>

namespace sim {
template <typename T, int D> using Vec = Eigen::Matrix<T, D, 1, 0, D, 1>;
template <typename T, int N, int M> using Mat = Eigen::Matrix<T, N, M, 0, N, M>;
} // namespace sim
