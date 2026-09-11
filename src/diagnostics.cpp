#include "diagnostics.hpp"

#include <Arduino.h>

#include "config.hpp"

namespace cmw::diagnostics {

#if CMW_DIAGNOSTICS
namespace {

const char* opticalStateName(OpticalState state) {
  switch (state) {
    case OpticalState::Blocked: return "BLOCKED";
    case OpticalState::Open: return "OPEN";
    case OpticalState::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char* lifecycleStateName(LifecycleState state) {
  switch (state) {
    case LifecycleState::Boot: return "BOOT";
    case LifecycleState::Shelf: return "SHELF";
    case LifecycleState::Active: return "ACTIVE";
  }
  return "BOOT";
}

const char* faultReasonName(FaultReason reason) {
  switch (reason) {
    case FaultReason::InvalidThresholds: return "invalid_thresholds";
    case FaultReason::AdcFailure: return "adc_failure";
    case FaultReason::AdcRail: return "adc_rail";
    case FaultReason::None: return "none";
  }
  return "unknown";
}

}  // namespace
#endif

void begin() {
#if CMW_DIAGNOSTICS
  Serial.begin(115200);
#endif
}

void emit(const DeviceSnapshot& r, bool ble_connected) {
#if CMW_DIAGNOSTICS
  Serial.print("{\"ambient_adc\":");
  Serial.print(r.sample.ambient);
  Serial.print(",\"lit_adc\":");
  Serial.print(r.sample.lit);
  Serial.print(",\"optical_delta\":");
  Serial.print(r.classification.delta);
  Serial.print(",\"optical_state\":\"");
  Serial.print(opticalStateName(r.classification.stable));
  Serial.print("\",\"counted_transition\":");
  Serial.print(r.counted_transition ? "true" : "false");
  Serial.print(",\"pulse_count\":");
  Serial.print(r.telemetry.pulse_count);
  Serial.print(",\"session_id\":");
  Serial.print(r.telemetry.session_id);
  Serial.print(",\"lifecycle_state\":\"");
  Serial.print(lifecycleStateName(r.lifecycle));
  Serial.print("\",\"ble_state\":\"");
  Serial.print(ble_connected ? "connected" : "advertising");
  Serial.print("\",\"battery_mv\":");
  Serial.print(r.telemetry.battery_mv);
  Serial.print(",\"open_threshold\":");
  Serial.print(config::kOpenThreshold);
  Serial.print(",\"blocked_threshold\":");
  Serial.print(config::kBlockedThreshold);
  Serial.print(",\"settle_us\":");
  Serial.print(config::kEmitterSettleUs);
  Serial.print(",\"receiver_settle_us\":");
  Serial.print(config::kReceiverSettleUs);
  Serial.print(",\"debounce_samples\":");
  Serial.print(config::kDebounceSamples);
  Serial.print(",\"firmware_build\":\"");
  Serial.print(config::kFirmwareBuild);
  Serial.print("\",\"sensor_fault\":");
  Serial.print(r.telemetry.sensor_fault ? "true" : "false");
  Serial.print(",\"fault_reason\":\"");
  Serial.print(faultReasonName(r.fault_reason));
  Serial.println("\"}");
#else
  (void)r;
  (void)ble_connected;
#endif
}

}  // namespace cmw::diagnostics
