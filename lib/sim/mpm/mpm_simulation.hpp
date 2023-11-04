#pragma once
#include "sim/simulation.hpp"

namespace sim::mpm {

template <class Real, int Dim> class MpmSimulation : public Simulation<Real, Dim> {
  using Base = Simulation<Real, Dim>;
  
};

} // namespace sim::mpm