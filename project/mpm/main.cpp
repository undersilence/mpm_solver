#include <Eigen/Core>
#include "sim/mpm/mpm_simulation.hpp"

int main() {
  using namespace sim::mpm;
  MpmSimulation<double, 3> mpm;
  mpm.execute(0, 1);
}