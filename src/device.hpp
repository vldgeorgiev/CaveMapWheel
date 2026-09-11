#pragma once

#include <array>
#include <cstdint>

#include "sensor.hpp"

namespace cmw {

enum class LifecycleState : std::uint8_t { Boot, Shelf, Active };
enum class Transition : std::uint8_t { None, ToActive, ToShelf };

struct DeviceConfig {
  SensorConfig sensor;
  std::uint32_t active_sample_period_ms;
  std::uint32_t shelf_probe_period_ms;
  std::uint32_t wake_window_ms;
  std::uint8_t wake_stable_changes;
  std::uint32_t inactivity_timeout_ms;
  std::uint32_t heartbeat_period_ms;
  std::uint32_t diagnostics_period_ms;
  std::uint32_t battery_period_ms;
  std::uint16_t battery_low_mv;
  std::uint16_t battery_recovered_mv;
  std::uint8_t telemetry_version;
};

struct DeviceEvent {
  Transition transition = Transition::None;
  bool counted = false;
};

struct TelemetrySnapshot {
  bool active = false;
  bool low_battery = false;
  bool sensor_fault = false;
  std::uint16_t session_id = 0;
  std::uint32_t pulse_count = 0;
  std::uint16_t battery_mv = 0;
};

struct DeviceSnapshot {
  TelemetrySnapshot telemetry;
  OpticalSample sample;
  Classification classification;
  bool counted_transition = false;
  LifecycleState lifecycle = LifecycleState::Boot;
  FaultReason fault_reason = FaultReason::None;
};

using TelemetryPacket = std::array<std::uint8_t, 10>;

bool updateLowBattery(bool current, std::uint16_t battery_mv,
                      std::uint16_t low_mv, std::uint16_t recovered_mv);
constexpr bool isCountedTransition(OpticalState previous, OpticalState current,
                                   bool active) {
  return active && previous == OpticalState::Blocked &&
         current == OpticalState::Open;
}
constexpr std::uint32_t incrementPulseCount(std::uint32_t count) {
  return count + 1U;
}
constexpr std::uint32_t modularDelta(std::uint32_t newer,
                                     std::uint32_t older) {
  return newer - older;
}
TelemetryPacket encodeTelemetry(const TelemetrySnapshot& snapshot,
                                std::uint8_t protocol_version = 1);

class Device {
 public:
  explicit Device(DeviceConfig config);

  Transition begin(std::uint32_t now, std::uint16_t session_id,
                   std::uint16_t battery_mv, bool start_active = false);
  Transition update(std::uint32_t now);
  DeviceEvent onSample(std::uint32_t now, const OpticalSample& sample);
  void updateBattery(std::uint16_t battery_mv);

  bool takeSampleDue(std::uint32_t now);
  bool takeHeartbeatDue(std::uint32_t now);
  bool takeBatteryDue(std::uint32_t now);
  bool takeDiagnosticsDue(std::uint32_t now);
  std::uint32_t shelfIdleMs(std::uint32_t now,
                            std::uint32_t maximum_ms) const;

  bool active() const { return state_ == LifecycleState::Active; }
  LifecycleState state() const { return state_; }
  DeviceSnapshot snapshot() const;
  TelemetryPacket telemetryPacket() const;

 private:
  void enterActive(std::uint32_t now);
  void enterShelf(std::uint32_t now);
  bool onShelfStableChange(std::uint32_t now);

  DeviceConfig config_;
  SensorState sensor_;
  LifecycleState state_ = LifecycleState::Boot;
  std::uint16_t session_id_ = 1;
  std::uint32_t pulse_count_ = 0;
  std::uint16_t battery_mv_ = 0;
  bool low_battery_ = false;
  OpticalSample last_sample_{};
  Classification last_classification_{};
  bool last_counted_ = false;
  std::uint32_t next_sample_ms_ = 0;
  std::uint32_t last_count_ms_ = 0;
  std::uint32_t gesture_started_ms_ = 0;
  std::uint8_t gesture_changes_ = 0;
  std::uint32_t next_heartbeat_ms_ = 0;
  std::uint32_t next_diagnostics_ms_ = 0;
  std::uint32_t next_battery_ms_ = 0;
};

}  // namespace cmw
