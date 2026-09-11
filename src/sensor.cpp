#include "sensor.hpp"

namespace cmw {

namespace {

void safeOff(OpticalIo& io) {
  io.setEmitter(false);
  io.setReceiverBias(false);
}

}  // namespace

OpticalSample acquireOpticalSample(OpticalIo& io,
                                   std::uint16_t receiver_settle_us,
                                   std::uint16_t emitter_settle_us) {
  OpticalSample sample;
  safeOff(io);
  io.setReceiverBias(true);
  io.settleMicroseconds(receiver_settle_us);
  if (!io.readReceiver(sample.ambient)) {
    safeOff(io);
    return sample;
  }

  io.setEmitter(true);
  io.settleMicroseconds(emitter_settle_us);
  if (!io.readReceiver(sample.lit)) {
    safeOff(io);
    return sample;
  }

  sample.valid = true;
  safeOff(io);
  return sample;
}

SensorState::SensorState(SensorConfig config)
    : config_(config), threshold_latched_(!validConfig()) {
  if (threshold_latched_) fault_reason_ = FaultReason::InvalidThresholds;
}

bool SensorState::validConfig() const {
  return config_.blocked_threshold < config_.open_threshold &&
         config_.debounce_samples > 0;
}

void SensorState::reset() {
  stable_ = OpticalState::Unknown;
  candidate_ = OpticalState::Unknown;
  candidate_count_ = 0;
  rail_tracking_ = false;
  valid_tracking_ = false;
  fault_reason_ = threshold_latched_ ? FaultReason::InvalidThresholds
                                     : FaultReason::None;
}

OpticalState SensorState::classify(std::int32_t delta) const {
  if (delta <= config_.blocked_threshold) return OpticalState::Blocked;
  if (delta >= config_.open_threshold) return OpticalState::Open;
  return stable_;
}

Classification SensorState::observe(const OpticalSample& sample,
                                    std::uint32_t now, bool active) {
  observeFault(sample, now, active);

  Classification result;
  result.previous = stable_;
  result.stable = stable_;
  if (!sample.valid) return result;
  result.delta = static_cast<std::int32_t>(sample.ambient) -
                 static_cast<std::int32_t>(sample.lit);
  if (!validConfig()) return result;
  const OpticalState observed = classify(result.delta);
  if (observed == OpticalState::Unknown || observed == stable_) {
    candidate_ = OpticalState::Unknown;
    candidate_count_ = 0;
    return result;
  }

  if (candidate_ != observed) {
    candidate_ = observed;
    candidate_count_ = 1;
  } else if (candidate_count_ < 0xff) {
    ++candidate_count_;
  }

  if (candidate_count_ >= config_.debounce_samples) {
    const OpticalState previous = stable_;
    stable_ = observed;
    candidate_ = OpticalState::Unknown;
    candidate_count_ = 0;
    result.previous = previous;
    result.stable = stable_;
    // Establishing the initial state is not a physical transition.
    result.stable_changed = previous != OpticalState::Unknown;
  }
  return result;
}

bool SensorState::isRail(const OpticalSample& sample) const {
  return sample.ambient <= config_.rail_low ||
         sample.ambient >= config_.rail_high ||
         sample.lit <= config_.rail_low || sample.lit >= config_.rail_high;
}

void SensorState::observeFault(const OpticalSample& sample, std::uint32_t now,
                               bool rail_detection_enabled) {
  if (threshold_latched_) {
    fault_reason_ = FaultReason::InvalidThresholds;
    return;
  }

  if (!sample.valid) {
    fault_reason_ = FaultReason::AdcFailure;
    rail_tracking_ = false;
    valid_tracking_ = false;
    return;
  }

  const bool railed = isRail(sample);
  if (!rail_detection_enabled && railed) {
    rail_tracking_ = false;
    valid_tracking_ = false;
    return;
  }

  if (rail_detection_enabled && railed) {
    valid_tracking_ = false;
    if (!rail_tracking_) {
      rail_tracking_ = true;
      rail_since_ms_ = now;
    } else if (intervalElapsed(now, rail_since_ms_,
                               config_.rail_duration_ms)) {
      fault_reason_ = FaultReason::AdcRail;
    }
    return;
  }

  rail_tracking_ = false;
  if (fault_reason_ == FaultReason::None) return;
  if (!valid_tracking_) {
    valid_tracking_ = true;
    valid_since_ms_ = now;
  } else if (intervalElapsed(now, valid_since_ms_,
                             config_.rail_duration_ms)) {
    fault_reason_ = FaultReason::None;
    valid_tracking_ = false;
  }
}

}  // namespace cmw
