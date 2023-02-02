// AOS layout so named with 'Particles'
#pragma once

#include "math/numeric_types.hpp"
#include <vector>

namespace sim {
// T: float/double, D: 2/3 Dim
template <typename T, int Dim> struct Particles {

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  
  std::vector<Vec<T, Dim>> X;      // Position
  std::vector<Vec<T, Dim>> V;      // Velocity
  std::vector<Vec<T, Dim>> M;      // Mass
  std::vector<Vec<T, Dim>> Vol;    // Volume
  std::vector<Mat<T, Dim, Dim>> F; // Deformation Gradient
  std::vector<Mat<T, Dim, Dim>> C; // Apic velocity affine
};

} // namespace sim