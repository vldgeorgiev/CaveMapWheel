#pragma once

#include <cstdint>

#include "device.hpp"

#ifndef CMW_DIAGNOSTICS
#define CMW_DIAGNOSTICS 1
#endif
#ifndef CMW_START_ACTIVE
#define CMW_START_ACTIVE 0
#endif
#ifndef CMW_BUILD_PROFILE
#define CMW_BUILD_PROFILE "unspecified"
#endif
#ifndef CMW_BLOCKED_THRESHOLD
#define CMW_BLOCKED_THRESHOLD 80
#endif
#ifndef CMW_OPEN_THRESHOLD
#define CMW_OPEN_THRESHOLD 240
#endif
#ifndef CMW_DEBOUNCE_SAMPLES
#define CMW_DEBOUNCE_SAMPLES 3
#endif
#ifndef CMW_ACTIVE_SAMPLE_PERIOD_MS
#define CMW_ACTIVE_SAMPLE_PERIOD_MS 5
#endif
#ifndef CMW_SHELF_PROBE_PERIOD_MS
#define CMW_SHELF_PROBE_PERIOD_MS 1000
#endif
#ifndef CMW_WAKE_WINDOW_MS
#define CMW_WAKE_WINDOW_MS 5000
#endif
#ifndef CMW_WAKE_STABLE_CHANGES
#define CMW_WAKE_STABLE_CHANGES 2
#endif
#ifndef CMW_INACTIVITY_TIMEOUT_MS
#define CMW_INACTIVITY_TIMEOUT_MS 900000UL
#endif
#ifndef CMW_BATTERY_LOW_MV
#define CMW_BATTERY_LOW_MV 3500
#endif
#ifndef CMW_BATTERY_RECOVERED_MV
#define CMW_BATTERY_RECOVERED_MV 3600
#endif
#ifndef CMW_BATTERY_DIVIDER_NUMERATOR
#define CMW_BATTERY_DIVIDER_NUMERATOR 1510
#endif
#ifndef CMW_BATTERY_DIVIDER_DENOMINATOR
#define CMW_BATTERY_DIVIDER_DENOMINATOR 510
#endif
#ifndef CMW_EMITTER_SETTLE_US
#define CMW_EMITTER_SETTLE_US 150
#endif
#ifndef CMW_RECEIVER_SETTLE_US
#define CMW_RECEIVER_SETTLE_US 150
#endif

namespace cmw::config {

constexpr std::int32_t kBlockedThreshold = CMW_BLOCKED_THRESHOLD;
constexpr std::int32_t kOpenThreshold = CMW_OPEN_THRESHOLD;
constexpr std::uint8_t kDebounceSamples = CMW_DEBOUNCE_SAMPLES;
constexpr std::uint32_t kActiveSamplePeriodMs = CMW_ACTIVE_SAMPLE_PERIOD_MS;
constexpr std::uint32_t kShelfProbePeriodMs = CMW_SHELF_PROBE_PERIOD_MS;
constexpr std::uint32_t kWakeWindowMs = CMW_WAKE_WINDOW_MS;
constexpr std::uint8_t kWakeStableChanges = CMW_WAKE_STABLE_CHANGES;
constexpr std::uint32_t kInactivityTimeoutMs = CMW_INACTIVITY_TIMEOUT_MS;
constexpr std::uint32_t kHeartbeatPeriodMs = 1000;
constexpr std::uint32_t kDiagnosticsPeriodMs = 125;
constexpr std::uint32_t kBatteryPeriodMs = 60000;
constexpr std::uint16_t kBatteryLowMv = CMW_BATTERY_LOW_MV;
constexpr std::uint16_t kBatteryRecoveredMv = CMW_BATTERY_RECOVERED_MV;
constexpr std::uint32_t kBatteryDividerNumerator =
    CMW_BATTERY_DIVIDER_NUMERATOR;
constexpr std::uint32_t kBatteryDividerDenominator =
    CMW_BATTERY_DIVIDER_DENOMINATOR;
constexpr std::uint16_t kEmitterSettleUs = CMW_EMITTER_SETTLE_US;
constexpr std::uint16_t kReceiverSettleUs = CMW_RECEIVER_SETTLE_US;
constexpr std::uint16_t kAdcRailLow = 2;
constexpr std::uint16_t kAdcRailHigh = 4093;
constexpr std::uint32_t kAdcRailFaultMs = 2000;
constexpr std::uint8_t kTelemetryVersion = 1;

constexpr char kDeviceName[] = "CaveMapWheel-V1";
constexpr char kServiceUuid[] = "64d60f54-cf51-4254-9cb6-3b01301fdea9";
constexpr char kTelemetryUuid[] = "92809e18-683a-466a-8555-c9040518d114";
constexpr char kFirmwareBuild[] = __DATE__ " " __TIME__ " " CMW_BUILD_PROFILE;

constexpr DeviceConfig kDeviceConfig{
    {kBlockedThreshold, kOpenThreshold, kDebounceSamples, kAdcRailLow,
     kAdcRailHigh, kAdcRailFaultMs},
    kActiveSamplePeriodMs,
    kShelfProbePeriodMs,
    kWakeWindowMs,
    kWakeStableChanges,
    kInactivityTimeoutMs,
    kHeartbeatPeriodMs,
    kDiagnosticsPeriodMs,
    kBatteryPeriodMs,
    kBatteryLowMv,
    kBatteryRecoveredMv,
    kTelemetryVersion};

static_assert(kActiveSamplePeriodMs == 5, "V1 sampling contract is 200 Hz");
static_assert(kDebounceSamples > 0, "debounce must use at least one sample");
static_assert(kWakeStableChanges >= 2, "wake requires two stable changes");
static_assert(kBatteryLowMv < kBatteryRecoveredMv,
              "battery thresholds must provide hysteresis");
static_assert(kBatteryDividerNumerator > kBatteryDividerDenominator,
              "battery divider scale must be greater than one");
static_assert(kBatteryDividerDenominator != 0,
              "battery divider denominator must be nonzero");
static_assert(kDiagnosticsPeriodMs >= 100 && kDiagnosticsPeriodMs <= 200,
              "diagnostics must remain between 5 and 10 Hz");

}  // namespace cmw::config
