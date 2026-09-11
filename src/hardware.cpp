#include "hardware.hpp"

#include <Adafruit_TinyUSB.h>
#include <nrf.h>

#include "config.hpp"

namespace cmw {

namespace {

constexpr std::uint32_t kAdcFullScale = 4095;
constexpr std::uint32_t kAdcReferenceMv = 3600;

bool readRandomByte(std::uint8_t& value) {
  NRF_RNG->EVENTS_VALRDY = 0;
  NRF_RNG->TASKS_START = 1;
  const std::uint32_t started = micros();
  while (NRF_RNG->EVENTS_VALRDY == 0) {
    if (static_cast<std::uint32_t>(micros() - started) >= 2000) return false;
  }
  value = NRF_RNG->VALUE;
  return true;
}

}  // namespace

void XiaoHardware::safeBegin() {
  digitalWrite(kEmitterPin, LOW);
  pinMode(kEmitterPin, OUTPUT);
  digitalWrite(kEmitterPin, LOW);
  digitalWrite(kReceiverBiasPin, LOW);
  pinMode(kReceiverBiasPin, OUTPUT);
  digitalWrite(kReceiverBiasPin, LOW);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  ledsOff();

  analogReadResolution(12);
  pinMode(VBAT_ENABLE, INPUT);  // Pending correction; see docs/validation.md.

#if !CMW_DIAGNOSTICS
  TinyUSBDevice.detach();
  NRF_USBD->ENABLE = 0;
  NRF_QSPI->ENABLE = 0;
#endif
}

void XiaoHardware::setEmitter(bool enabled) {
  digitalWrite(kEmitterPin, enabled ? HIGH : LOW);
}

void XiaoHardware::setReceiverBias(bool enabled) {
  digitalWrite(kReceiverBiasPin, enabled ? HIGH : LOW);
}

void XiaoHardware::settleMicroseconds(std::uint16_t delay_us) {
  delayMicroseconds(delay_us);
}

bool XiaoHardware::readReceiver(std::uint16_t& value) {
  const int reading = analogRead(kReceiverAdcPin);
  if (reading < 0 || reading > static_cast<int>(kAdcFullScale)) return false;
  value = static_cast<std::uint16_t>(reading);
  return true;
}

std::uint16_t XiaoHardware::readBatteryMv() {
  digitalWrite(VBAT_ENABLE, LOW);
  pinMode(VBAT_ENABLE, OUTPUT);
  delayMicroseconds(200);
  const std::uint32_t raw = analogRead(PIN_VBAT);
  pinMode(VBAT_ENABLE, INPUT);  // Pending correction; see docs/validation.md.
  const std::uint64_t scaled =
      static_cast<std::uint64_t>(raw) * kAdcReferenceMv *
      config::kBatteryDividerNumerator;
  return static_cast<std::uint16_t>(
      (scaled + (kAdcFullScale * config::kBatteryDividerDenominator) / 2) /
      (kAdcFullScale * config::kBatteryDividerDenominator));
}

std::uint16_t XiaoHardware::newSessionId() const {
  std::uint8_t low = 0;
  std::uint8_t high = 0;
  const bool random_ok = readRandomByte(low) && readRandomByte(high);
  NRF_RNG->TASKS_STOP = 1;
  std::uint16_t session = static_cast<std::uint16_t>(low | (high << 8));
  if (!random_ok) {
    const std::uint32_t mixed = NRF_FICR->DEVICEID[0] ^ NRF_FICR->DEVICEID[1] ^
                                micros();
    session = static_cast<std::uint16_t>(mixed ^ (mixed >> 16));
  }
  return session == 0 ? 1 : session;
}

void XiaoHardware::showWakeIndication(bool low_battery) {
  ledsOff();
  digitalWrite(low_battery ? LED_RED : LED_GREEN, LOW);
  delay(80);
  ledsOff();
}

void XiaoHardware::ledsOff() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

void XiaoHardware::idle(std::uint32_t milliseconds) {
  if (milliseconds == 0) {
    yield();
  } else {
    delay(milliseconds);
  }
}

}  // namespace cmw
