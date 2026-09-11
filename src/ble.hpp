#pragma once

#include <bluefruit.h>

#include "device.hpp"

namespace cmw {

class BleTransport {
 public:
  BleTransport();
  void begin();
  void stop();
  void publish(const TelemetryPacket& packet, bool notify);
  bool connected() const;

 private:
  BLEService service_;
  BLECharacteristic telemetry_;
  bool initialized_ = false;
  bool running_ = false;
};

}  // namespace cmw
