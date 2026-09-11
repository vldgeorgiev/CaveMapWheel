#include "ble.hpp"

#include "config.hpp"

namespace cmw {

BleTransport::BleTransport()
    : service_(config::kServiceUuid), telemetry_(config::kTelemetryUuid) {}

void BleTransport::begin() {
  if (!initialized_) {
    Bluefruit.begin();
    Bluefruit.setName(config::kDeviceName);
    service_.begin();
    telemetry_.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    telemetry_.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    telemetry_.setFixedLen(10);
    telemetry_.begin();
    initialized_ = true;
  }
  if (running_) return;
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.ScanResponse.addService(service_);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.start(0);
  running_ = true;
}

void BleTransport::stop() {
  if (!initialized_ || !running_) return;
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.stop();
  if (Bluefruit.connected()) Bluefruit.disconnect(Bluefruit.connHandle());
  running_ = false;
}

void BleTransport::publish(const TelemetryPacket& packet, bool notify) {
  if (!running_) return;
  telemetry_.write(packet.data(), packet.size());
  if (notify && Bluefruit.connected()) {
    telemetry_.notify(packet.data(), packet.size());
  }
}

bool BleTransport::connected() const {
  return running_ && Bluefruit.connected();
}

}  // namespace cmw
