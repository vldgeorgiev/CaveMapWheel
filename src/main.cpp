#include <Arduino.h>

#include "ble.hpp"
#include "config.hpp"
#include "device.hpp"
#include "diagnostics.hpp"
#include "hardware.hpp"
#include "sensor.hpp"

namespace {

cmw::XiaoHardware hardware;
cmw::BleTransport ble;
cmw::Device device(cmw::config::kDeviceConfig);

void publish(bool notify) {
  ble.publish(device.telemetryPacket(), notify);
}

void applyTransition(cmw::Transition transition) {
  if (transition == cmw::Transition::ToActive) {
    ble.begin();
    hardware.showWakeIndication(device.snapshot().telemetry.low_battery);
    publish(false);
  } else if (transition == cmw::Transition::ToShelf) {
    ble.stop();
    hardware.setEmitter(false);
    hardware.setReceiverBias(false);
    hardware.ledsOff();
  }
}

}  // namespace

void setup() {
  hardware.safeBegin();
  const std::uint16_t session_id = hardware.newSessionId();
  cmw::diagnostics::begin();
  const std::uint32_t now = millis();
  const std::uint16_t battery_mv = hardware.readBatteryMv();
  applyTransition(device.begin(now, session_id, battery_mv,
                               CMW_START_ACTIVE != 0));
}

void loop() {
  const std::uint32_t now = millis();
  applyTransition(device.update(now));

  if (device.takeSampleDue(now)) {
    const cmw::OpticalSample sample = cmw::acquireOpticalSample(
        hardware, cmw::config::kReceiverSettleUs,
        cmw::config::kEmitterSettleUs);
    const cmw::DeviceEvent event = device.onSample(now, sample);
    applyTransition(event.transition);
    if (event.counted) publish(true);
  }

  if (device.active()) {
    // Preserve V1 ordering when several deadlines are due simultaneously.
    if (device.takeHeartbeatDue(now)) publish(true);
    if (device.takeBatteryDue(now)) {
      device.updateBattery(hardware.readBatteryMv());
      publish(false);
    }
#if CMW_DIAGNOSTICS
    if (device.takeDiagnosticsDue(now)) {
      cmw::diagnostics::emit(device.snapshot(), ble.connected());
    }
#endif
  }

  const std::uint32_t idle_now = millis();
  hardware.idle(device.active() ? 1 : device.shelfIdleMs(idle_now, 1000));
}
