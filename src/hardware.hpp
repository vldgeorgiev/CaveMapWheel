#pragma once

#include <Arduino.h>

#include "sensor.hpp"

#ifndef CMW_RECEIVER_ADC_PIN
#define CMW_RECEIVER_ADC_PIN D0
#endif
#ifndef CMW_EMITTER_PIN
#define CMW_EMITTER_PIN D1
#endif
#ifndef CMW_RECEIVER_BIAS_PIN
#define CMW_RECEIVER_BIAS_PIN D2
#endif

namespace cmw {

class XiaoHardware final : public OpticalIo {
 public:
  void safeBegin();
  void setEmitter(bool enabled) override;
  void setReceiverBias(bool enabled) override;
  void settleMicroseconds(std::uint16_t delay_us) override;
  bool readReceiver(std::uint16_t& value) override;

  std::uint16_t readBatteryMv();
  std::uint16_t newSessionId() const;
  void showWakeIndication(bool low_battery);
  void ledsOff();
  void idle(std::uint32_t milliseconds);

 private:
  static constexpr std::uint8_t kReceiverAdcPin = CMW_RECEIVER_ADC_PIN;
  static constexpr std::uint8_t kEmitterPin = CMW_EMITTER_PIN;
  static constexpr std::uint8_t kReceiverBiasPin = CMW_RECEIVER_BIAS_PIN;
};

}  // namespace cmw
