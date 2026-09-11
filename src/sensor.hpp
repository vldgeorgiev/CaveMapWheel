#pragma once

#include <cstdint>

namespace cmw {

enum class OpticalState : std::uint8_t { Unknown, Blocked, Open };
enum class FaultReason : std::uint8_t {
  None,
  InvalidThresholds,
  AdcFailure,
  AdcRail,
};

struct OpticalSample {
  std::uint16_t ambient = 0;
  std::uint16_t lit = 0;
  bool valid = false;
};

struct Classification {
  std::int32_t delta = 0;
  OpticalState stable = OpticalState::Unknown;
  bool stable_changed = false;
  OpticalState previous = OpticalState::Unknown;
};

constexpr bool deadlineReached(std::uint32_t now, std::uint32_t deadline) {
  return static_cast<std::int32_t>(now - deadline) >= 0;
}

constexpr bool intervalElapsed(std::uint32_t now, std::uint32_t since,
                               std::uint32_t interval) {
  return static_cast<std::uint32_t>(now - since) >= interval;
}

class OpticalIo {
 public:
  virtual ~OpticalIo() = default;
  virtual void setEmitter(bool enabled) = 0;
  virtual void setReceiverBias(bool enabled) = 0;
  virtual void settleMicroseconds(std::uint16_t delay_us) = 0;
  virtual bool readReceiver(std::uint16_t& value) = 0;
};

OpticalSample acquireOpticalSample(OpticalIo& io,
                                   std::uint16_t receiver_settle_us,
                                   std::uint16_t emitter_settle_us);

struct SensorConfig {
  std::int32_t blocked_threshold;
  std::int32_t open_threshold;
  std::uint8_t debounce_samples;
  std::uint16_t rail_low;
  std::uint16_t rail_high;
  std::uint32_t rail_duration_ms;
};

class SensorState {
 public:
  explicit SensorState(SensorConfig config);

  void reset();
  Classification observe(const OpticalSample& sample, std::uint32_t now,
                         bool active);
  bool validConfig() const;
  OpticalState state() const { return stable_; }
  FaultReason faultReason() const { return fault_reason_; }
  bool faulted() const { return fault_reason_ != FaultReason::None; }

 private:
  OpticalState classify(std::int32_t delta) const;
  void observeFault(const OpticalSample& sample, std::uint32_t now,
                    bool rail_detection_enabled);
  bool isRail(const OpticalSample& sample) const;

  SensorConfig config_;
  OpticalState stable_ = OpticalState::Unknown;
  OpticalState candidate_ = OpticalState::Unknown;
  std::uint8_t candidate_count_ = 0;
  bool threshold_latched_ = false;
  FaultReason fault_reason_ = FaultReason::None;
  bool rail_tracking_ = false;
  std::uint32_t rail_since_ms_ = 0;
  bool valid_tracking_ = false;
  std::uint32_t valid_since_ms_ = 0;
};

}  // namespace cmw
