#include <unity.h>

#include "device.hpp"

using namespace cmw;

namespace {

void test_only_active_blocked_to_open_counts() {
  TEST_ASSERT_FALSE(
      isCountedTransition(OpticalState::Open, OpticalState::Blocked, true));
  TEST_ASSERT_FALSE(
      isCountedTransition(OpticalState::Blocked, OpticalState::Open, false));
  TEST_ASSERT_TRUE(
      isCountedTransition(OpticalState::Blocked, OpticalState::Open, true));
}

void test_counter_rollover_and_delta() {
  TEST_ASSERT_EQUAL_UINT32(0, incrementPulseCount(0xffffffffU));
  TEST_ASSERT_EQUAL_UINT32(3, modularDelta(1, 0xfffffffeU));
}

void test_golden_telemetry_vector() {
  const TelemetrySnapshot snapshot{true, true, true, 0xabcd, 0x87654321,
                                   0x1234};
  const auto bytes = encodeTelemetry(snapshot);
  const std::uint8_t expected[10] = {1,    7,    0xcd, 0xab, 0x21,
                                     0x43, 0x65, 0x87, 0x34, 0x12};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, bytes.data(), bytes.size());
  TEST_ASSERT_EQUAL_UINT32(10, bytes.size());
}

void test_zero_and_maximum_vectors() {
  auto zero = encodeTelemetry({false, false, false, 0, 0, 0});
  const std::uint8_t expected_zero[10] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_zero, zero.data(), zero.size());
  auto maximum = encodeTelemetry(
      {false, false, false, 0xffff, 0xffffffffU, 0xffff});
  TEST_ASSERT_EQUAL_UINT8(1, maximum[0]);
  TEST_ASSERT_EQUAL_UINT8(0, maximum[1]);
  for (std::size_t i = 2; i < maximum.size(); ++i) {
    TEST_ASSERT_EQUAL_HEX8(0xff, maximum[i]);
  }
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_only_active_blocked_to_open_counts);
  RUN_TEST(test_counter_rollover_and_delta);
  RUN_TEST(test_golden_telemetry_vector);
  RUN_TEST(test_zero_and_maximum_vectors);
  return UNITY_END();
}
