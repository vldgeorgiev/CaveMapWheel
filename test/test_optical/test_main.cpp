#include <unity.h>

#include <string>
#include <vector>

#include "sensor.hpp"

using namespace cmw;

namespace {

class FakeIo : public OpticalIo {
 public:
  void setEmitter(bool enabled) override {
    calls += enabled ? "E1 " : "E0 ";
  }
  void setReceiverBias(bool enabled) override {
    calls += enabled ? "R1 " : "R0 ";
  }
  void settleMicroseconds(std::uint16_t) override { calls += "S "; }
  bool readReceiver(std::uint16_t& value) override {
    calls += "A ";
    if (read_index >= readings.size()) return false;
    const int next = readings[read_index++];
    if (next < 0) return false;
    value = static_cast<std::uint16_t>(next);
    return true;
  }

  std::vector<int> readings;
  std::size_t read_index = 0;
  std::string calls;
};

void test_differential_and_debounce() {
  SensorState sensor({80, 240, 3, 2, 4093, 2000});
  TEST_ASSERT_TRUE(sensor.validConfig());
  TEST_ASSERT_EQUAL_INT32(
      300, sensor.observe({500, 200, true}, 0, true).delta);
  sensor.observe({500, 200, true}, 1, true);
  const auto established = sensor.observe({500, 200, true}, 2, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OpticalState::Open),
                        static_cast<int>(established.stable));
  TEST_ASSERT_FALSE(established.stable_changed);

  sensor.observe({300, 150, true}, 3, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OpticalState::Open),
                        static_cast<int>(sensor.state()));
  sensor.observe({150, 100, true}, 4, true);
  sensor.observe({150, 100, true}, 5, true);
  TEST_ASSERT_FALSE(
      sensor.observe({500, 200, true}, 6, true).stable_changed);
  sensor.observe({150, 100, true}, 7, true);
  sensor.observe({150, 100, true}, 8, true);
  const auto blocked = sensor.observe({150, 100, true}, 9, true);
  TEST_ASSERT_TRUE(blocked.stable_changed);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OpticalState::Blocked),
                        static_cast<int>(blocked.stable));
}

void test_invalid_thresholds() {
  SensorState sensor({240, 240, 3, 2, 4093, 2000});
  TEST_ASSERT_FALSE(sensor.validConfig());
  const auto result = sensor.observe({500, 100, true}, 0, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OpticalState::Unknown),
                        static_cast<int>(result.stable));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::InvalidThresholds),
                        static_cast<int>(sensor.faultReason()));
}

void test_acquisition_order_and_cleanup() {
  FakeIo io;
  io.readings = {700, 300};
  const auto sample = acquireOpticalSample(io, 10, 20);
  TEST_ASSERT_TRUE(sample.valid);
  TEST_ASSERT_EQUAL_UINT16(700, sample.ambient);
  TEST_ASSERT_EQUAL_UINT16(300, sample.lit);
  TEST_ASSERT_EQUAL_STRING("E0 R0 R1 S A E1 S A E0 R0 ", io.calls.c_str());
}

void test_second_acquisition_failure_is_safe() {
  FakeIo io;
  io.readings = {700, -1};
  TEST_ASSERT_FALSE(acquireOpticalSample(io, 10, 20).valid);
  TEST_ASSERT_EQUAL_STRING("E0 R0 R1 S A E1 S A E0 R0 ", io.calls.c_str());
}

void test_first_acquisition_failure_is_safe() {
  FakeIo io;
  io.readings = {-1};
  TEST_ASSERT_FALSE(acquireOpticalSample(io, 10, 20).valid);
  TEST_ASSERT_EQUAL_STRING("E0 R0 R1 S A E0 R0 ", io.calls.c_str());
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_differential_and_debounce);
  RUN_TEST(test_invalid_thresholds);
  RUN_TEST(test_acquisition_order_and_cleanup);
  RUN_TEST(test_second_acquisition_failure_is_safe);
  RUN_TEST(test_first_acquisition_failure_is_safe);
  return UNITY_END();
}
