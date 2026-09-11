#include <unity.h>

#include "device.hpp"

using namespace cmw;

namespace {

DeviceConfig testConfig() {
  return {{80, 240, 1, 2, 4093, 2000},
          5,
          1000,
          5000,
          2,
          100,
          10,
          5,
          20,
          3500,
          3600,
          1};
}

constexpr OpticalSample kBlocked{200, 150, true};
constexpr OpticalSample kOpen{500, 100, true};

void test_full_lifecycle_retains_session_and_count() {
  Device device(testConfig());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::ToShelf),
                        static_cast<int>(device.begin(0, 7, 3700)));
  TEST_ASSERT_EQUAL_UINT16(7, device.snapshot().telemetry.session_id);
  TEST_ASSERT_EQUAL_UINT32(0, device.snapshot().telemetry.pulse_count);

  TEST_ASSERT_FALSE(device.onSample(0, kBlocked).counted);  // establish
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(Transition::None),
      static_cast<int>(device.onSample(1000, kOpen).transition));
  const DeviceEvent wake = device.onSample(6000, kBlocked);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::ToActive),
                        static_cast<int>(wake.transition));
  TEST_ASSERT_FALSE(wake.counted);
  TEST_ASSERT_EQUAL_UINT32(0, device.snapshot().telemetry.pulse_count);

  const DeviceEvent count = device.onSample(6005, kOpen);
  TEST_ASSERT_TRUE(count.counted);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::None),
                        static_cast<int>(count.transition));
  TEST_ASSERT_EQUAL_UINT32(1, device.snapshot().telemetry.pulse_count);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::None),
                        static_cast<int>(device.update(6104)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::ToShelf),
                        static_cast<int>(device.update(6105)));

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(Transition::None),
      static_cast<int>(device.onSample(6110, kBlocked).transition));
  const DeviceEvent second_wake = device.onSample(6200, kOpen);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Transition::ToActive),
                        static_cast<int>(second_wake.transition));
  TEST_ASSERT_FALSE(second_wake.counted);
  TEST_ASSERT_EQUAL_UINT16(7, device.snapshot().telemetry.session_id);
  TEST_ASSERT_EQUAL_UINT32(1, device.snapshot().telemetry.pulse_count);
}

void test_wake_window_boundary() {
  Device exact(testConfig());
  exact.begin(0, 1, 3700);
  exact.onSample(0, kBlocked);
  exact.onSample(100, kOpen);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(Transition::ToActive),
      static_cast<int>(exact.onSample(5100, kBlocked).transition));

  Device expired(testConfig());
  expired.begin(0, 1, 3700);
  expired.onSample(0, kBlocked);
  expired.onSample(100, kOpen);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(Transition::None),
      static_cast<int>(expired.onSample(5101, kBlocked).transition));
  TEST_ASSERT_FALSE(expired.active());
}

void test_deadlines_do_not_catch_up() {
  Device device(testConfig());
  device.begin(100, 1, 3700, true);

  TEST_ASSERT_TRUE(device.takeHeartbeatDue(100));
  TEST_ASSERT_FALSE(device.takeHeartbeatDue(100));
  TEST_ASSERT_TRUE(device.takeDiagnosticsDue(100));
  TEST_ASSERT_FALSE(device.takeDiagnosticsDue(100));
  TEST_ASSERT_FALSE(device.takeBatteryDue(119));
  TEST_ASSERT_TRUE(device.takeBatteryDue(120));
  TEST_ASSERT_FALSE(device.takeBatteryDue(120));

  TEST_ASSERT_TRUE(device.takeHeartbeatDue(1000));
  TEST_ASSERT_FALSE(device.takeHeartbeatDue(1000));
  TEST_ASSERT_TRUE(device.takeDiagnosticsDue(1000));
  TEST_ASSERT_FALSE(device.takeDiagnosticsDue(1000));
  TEST_ASSERT_TRUE(device.takeBatteryDue(1000));
  TEST_ASSERT_FALSE(device.takeBatteryDue(1000));
}

void test_sample_schedule_is_wrap_safe_and_has_no_catch_up() {
  Device device(testConfig());
  device.begin(0xfffffff0U, 1, 3700, true);
  TEST_ASSERT_TRUE(device.takeSampleDue(0xfffffff0U));
  TEST_ASSERT_FALSE(device.takeSampleDue(0xfffffff1U));
  TEST_ASSERT_TRUE(device.takeSampleDue(0xfffffff5U));
  TEST_ASSERT_TRUE(device.takeSampleDue(1000));
  TEST_ASSERT_FALSE(device.takeSampleDue(1000));
}

void test_reset_starts_new_zero_count_session() {
  Device device(testConfig());
  device.begin(0, 7, 3700, true);
  device.onSample(0, kBlocked);
  TEST_ASSERT_TRUE(device.onSample(5, kOpen).counted);
  device.begin(10, 0, 3700);
  TEST_ASSERT_EQUAL_UINT16(1, device.snapshot().telemetry.session_id);
  TEST_ASSERT_EQUAL_UINT32(0, device.snapshot().telemetry.pulse_count);
}

void test_four_openings_produce_four_counts() {
  Device device(testConfig());
  device.begin(0, 1, 3700, true);
  device.onSample(0, kBlocked);  // establish without counting
  for (std::uint32_t i = 0; i < 4; ++i) {
    TEST_ASSERT_TRUE(device.onSample(5 + i * 10, kOpen).counted);
    TEST_ASSERT_FALSE(device.onSample(10 + i * 10, kBlocked).counted);
  }
  TEST_ASSERT_EQUAL_UINT32(4, device.snapshot().telemetry.pulse_count);
}

void test_invalid_sample_keeps_last_classification_for_diagnostics() {
  Device device(testConfig());
  device.begin(0, 1, 3700, true);
  device.onSample(0, kOpen);
  TEST_ASSERT_EQUAL_INT32(400, device.snapshot().classification.delta);
  device.onSample(5, {0, 0, false});
  const DeviceSnapshot snapshot = device.snapshot();
  TEST_ASSERT_EQUAL_INT32(400, snapshot.classification.delta);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::AdcFailure),
                        static_cast<int>(snapshot.fault_reason));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_full_lifecycle_retains_session_and_count);
  RUN_TEST(test_wake_window_boundary);
  RUN_TEST(test_deadlines_do_not_catch_up);
  RUN_TEST(test_sample_schedule_is_wrap_safe_and_has_no_catch_up);
  RUN_TEST(test_reset_starts_new_zero_count_session);
  RUN_TEST(test_four_openings_produce_four_counts);
  RUN_TEST(test_invalid_sample_keeps_last_classification_for_diagnostics);
  return UNITY_END();
}
