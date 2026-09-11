#include "device.hpp"

namespace cmw {

bool updateLowBattery(bool current, std::uint16_t battery_mv,
                      std::uint16_t low_mv, std::uint16_t recovered_mv) {
  if (low_mv >= recovered_mv) return current;
  if (current) return battery_mv <= recovered_mv;
  return battery_mv < low_mv;
}

TelemetryPacket encodeTelemetry(const TelemetrySnapshot& snapshot,
                                std::uint8_t protocol_version) {
  TelemetryPacket bytes{};
  bytes[0] = protocol_version;
  bytes[1] = static_cast<std::uint8_t>((snapshot.active ? 0x01 : 0) |
                                      (snapshot.low_battery ? 0x02 : 0) |
                                      (snapshot.sensor_fault ? 0x04 : 0));
  bytes[2] = static_cast<std::uint8_t>(snapshot.session_id);
  bytes[3] = static_cast<std::uint8_t>(snapshot.session_id >> 8);
  bytes[4] = static_cast<std::uint8_t>(snapshot.pulse_count);
  bytes[5] = static_cast<std::uint8_t>(snapshot.pulse_count >> 8);
  bytes[6] = static_cast<std::uint8_t>(snapshot.pulse_count >> 16);
  bytes[7] = static_cast<std::uint8_t>(snapshot.pulse_count >> 24);
  bytes[8] = static_cast<std::uint8_t>(snapshot.battery_mv);
  bytes[9] = static_cast<std::uint8_t>(snapshot.battery_mv >> 8);
  return bytes;
}

Device::Device(DeviceConfig config) : config_(config), sensor_(config.sensor) {}

Transition Device::begin(std::uint32_t now, std::uint16_t session_id,
                         std::uint16_t battery_mv, bool start_active) {
  sensor_.reset();
  state_ = LifecycleState::Boot;
  session_id_ = session_id == 0 ? 1 : session_id;
  pulse_count_ = 0;
  low_battery_ = false;
  last_sample_ = {};
  last_classification_ = {};
  last_counted_ = false;
  updateBattery(battery_mv);
  if (start_active) {
    enterActive(now);
    return Transition::ToActive;
  }
  enterShelf(now);
  return Transition::ToShelf;
}

void Device::enterActive(std::uint32_t now) {
  state_ = LifecycleState::Active;
  next_sample_ms_ = now;
  last_count_ms_ = now;
  gesture_changes_ = 0;
  next_heartbeat_ms_ = now;
  next_diagnostics_ms_ = now;
  next_battery_ms_ = now + config_.battery_period_ms;
}

void Device::enterShelf(std::uint32_t now) {
  state_ = LifecycleState::Shelf;
  next_sample_ms_ = now;
  gesture_changes_ = 0;
}

Transition Device::update(std::uint32_t now) {
  if (active() &&
      intervalElapsed(now, last_count_ms_, config_.inactivity_timeout_ms)) {
    enterShelf(now);
    return Transition::ToShelf;
  }
  return Transition::None;
}

bool Device::takeSampleDue(std::uint32_t now) {
  if (state_ == LifecycleState::Boot ||
      !deadlineReached(now, next_sample_ms_)) {
    return false;
  }
  const std::uint32_t period = active() ? config_.active_sample_period_ms
                                        : config_.shelf_probe_period_ms;
  next_sample_ms_ = now + period;
  return true;
}

std::uint32_t Device::shelfIdleMs(std::uint32_t now,
                                  std::uint32_t maximum_ms) const {
  if (state_ == LifecycleState::Boot || deadlineReached(now, next_sample_ms_)) {
    return 0;
  }
  const std::uint32_t remaining = next_sample_ms_ - now;
  return remaining < maximum_ms ? remaining : maximum_ms;
}

bool Device::onShelfStableChange(std::uint32_t now) {
  if (state_ != LifecycleState::Shelf) return false;
  if (gesture_changes_ == 0 ||
      intervalElapsed(now, gesture_started_ms_, config_.wake_window_ms + 1U)) {
    gesture_started_ms_ = now;
    gesture_changes_ = 1;
    return false;
  }
  ++gesture_changes_;
  if (gesture_changes_ < config_.wake_stable_changes) return false;
  enterActive(now);
  return true;
}

DeviceEvent Device::onSample(std::uint32_t now, const OpticalSample& sample) {
  last_sample_ = sample;
  last_counted_ = false;
  const Classification classification = sensor_.observe(sample, now, active());
  if (!sample.valid) return {};
  last_classification_ = classification;
  if (!classification.stable_changed) return {};

  if (active()) {
    if (isCountedTransition(classification.previous,
                            classification.stable, true)) {
      pulse_count_ = incrementPulseCount(pulse_count_);
      last_count_ms_ = now;
      last_counted_ = true;
      return {Transition::None, true};
    }
    return {};
  }

  if (onShelfStableChange(now)) return {Transition::ToActive, false};
  return {};
}

void Device::updateBattery(std::uint16_t battery_mv) {
  battery_mv_ = battery_mv;
  low_battery_ = updateLowBattery(low_battery_, battery_mv,
                                  config_.battery_low_mv,
                                  config_.battery_recovered_mv);
}

bool Device::takeHeartbeatDue(std::uint32_t now) {
  if (!active() || !deadlineReached(now, next_heartbeat_ms_)) return false;
  next_heartbeat_ms_ = now + config_.heartbeat_period_ms;
  return true;
}

bool Device::takeBatteryDue(std::uint32_t now) {
  if (!active() || !deadlineReached(now, next_battery_ms_)) return false;
  next_battery_ms_ = now + config_.battery_period_ms;
  return true;
}

bool Device::takeDiagnosticsDue(std::uint32_t now) {
  if (!active() || !deadlineReached(now, next_diagnostics_ms_)) return false;
  next_diagnostics_ms_ = now + config_.diagnostics_period_ms;
  return true;
}

DeviceSnapshot Device::snapshot() const {
  DeviceSnapshot value;
  value.telemetry = {active(), low_battery_, sensor_.faulted(), session_id_,
                     pulse_count_, battery_mv_};
  value.sample = last_sample_;
  value.classification = last_classification_;
  value.counted_transition = last_counted_;
  value.lifecycle = state_;
  value.fault_reason = sensor_.faultReason();
  return value;
}

TelemetryPacket Device::telemetryPacket() const {
  return encodeTelemetry(snapshot().telemetry, config_.telemetry_version);
}

}  // namespace cmw
