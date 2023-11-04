#pragma once

#include <cstdint>
#include <utils/logger.hpp>

namespace sim {

template <class Real, int Dim> class Simulation {
public:
  virtual void execute(int32_t start_frame = 0, int32_t end_frame = 0);
  virtual void tick_step(Real dt);
  virtual void tick_frame(int32_t frame_idx);
  virtual void reload(int32_t frame_idx);

  virtual void write(int32_t frame_idx) const;
  virtual Real calc_dt() const { return max_dt; };
  virtual std::string_view sim_name() const { return "simulation"; }

private:
  Real max_dt = 1e-3;
  Real frame_rate = 60; // 30fps or 60fps configuration
};

#pragma region Simulation_Definitions

template <class Real, int Dim>
void Simulation<Real, Dim>::execute(int32_t start_frame, int32_t end_frame) {
  reload(start_frame - 1);
  for (int i = start_frame; i < end_frame; ++i) {
    tick_frame(start_frame);
  }
}

template <class Real, int Dim> void Simulation<Real, Dim>::tick_frame(int32_t frame_idx) {
  const Real frame_time = 1. / frame_rate;
  Real sum_dt = 0;

  LOG_INFO("Tick Frame {}", frame_idx);

  while (sum_dt < frame_time) {
    auto dt = calc_dt();
    if (dt + sum_dt > frame_time) {
      dt = frame_time - sum_dt;
    } else if (dt * 2 + sum_dt > frame_time) {
      dt = (frame_time - sum_dt) * 0.5f;
    }

    tick_step(dt);
    sum_dt += dt;
  }

  write(frame_idx);
}

template <class Real, int Dim> void Simulation<Real, Dim>::tick_step(Real dt) {
  LOG_INFO("Tick Step delta_time: {}", dt);
}

template <class Real, int Dim> void Simulation<Real, Dim>::write(int32_t frame_idx) const {
  LOG_DEBUG("Write Frame {}", frame_idx);
}

template <class Real, int Dim> void Simulation<Real, Dim>::reload(int32_t frame_idx) {
  if (frame_idx <= 0)
    return;
}

#pragma endregion Simulation_Definitions

} // namespace sim