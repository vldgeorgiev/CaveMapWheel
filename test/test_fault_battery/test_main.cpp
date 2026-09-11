#include <unity.h>

#include "device.hpp"

using namespace cmw;

namespace {

void test_battery_hysteresis() {
  bool low = false;
  TEST_ASSERT_FALSE(low = updateLowBattery(low, 3700, 3500, 3600));
  TEST_ASSERT_FALSE(low = updateLowBattery(low, 3500, 3500, 3600));
  TEST_ASSERT_TRUE(low = updateLowBattery(low, 3499, 3500, 3600));
  TEST_ASSERT_TRUE(low = updateLowBattery(low, 3599, 3500, 3600));
  TEST_ASSERT_TRUE(low = updateLowBattery(low, 3600, 3500, 3600));
  TEST_ASSERT_FALSE(low = updateLowBattery(low, 3601, 3500, 3600));
}

void test_invalid_threshold_fault_is_latched() {
  SensorState sensor({240, 240, 3, 2, 4093, 2000});
  sensor.observe({100, 200, true}, 0, true);
  sensor.observe({100, 200, true}, 3000, true);
  TEST_ASSERT_TRUE(sensor.faulted());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::InvalidThresholds),
                        static_cast<int>(sensor.faultReason()));
}

void test_adc_failure_clears_after_valid_sequence() {
  SensorState sensor({80, 240, 3, 2, 4093, 2000});
  sensor.observe({0, 0, false}, 10, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::AdcFailure),
                        static_cast<int>(sensor.faultReason()));
  sensor.observe({100, 200, true}, 20, true);
  sensor.observe({100, 200, true}, 2019, true);
  TEST_ASSERT_TRUE(sensor.faulted());
  sensor.observe({100, 200, true}, 2020, true);
  TEST_ASSERT_FALSE(sensor.faulted());
}

void test_continuous_rail_fault_and_clear() {
  SensorState sensor({80, 240, 3, 2, 4093, 2000});
  sensor.observe({0, 100, true}, 100, true);
  sensor.observe({0, 100, true}, 2099, true);
  TEST_ASSERT_FALSE(sensor.faulted());
  sensor.observe({0, 100, true}, 2100, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::AdcRail),
                        static_cast<int>(sensor.faultReason()));
  sensor.observe({100, 200, true}, 2200, true);
  sensor.observe({100, 200, true}, 4200, true);
  TEST_ASSERT_FALSE(sensor.faulted());
}

void test_shelf_rail_does_not_clear_active_fault() {
  SensorState sensor({80, 240, 3, 2, 4093, 2000});
  sensor.observe({0, 100, true}, 100, true);
  sensor.observe({0, 100, true}, 2100, true);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::AdcRail),
                        static_cast<int>(sensor.faultReason()));
  sensor.observe({0, 100, true}, 2200, false);
  sensor.observe({0, 100, true}, 5000, false);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultReason::AdcRail),
                        static_cast<int>(sensor.faultReason()));
  sensor.observe({100, 200, true}, 5100, false);
  sensor.observe({100, 200, true}, 7100, false);
  TEST_ASSERT_FALSE(sensor.faulted());
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_battery_hysteresis);
  RUN_TEST(test_invalid_threshold_fault_is_latched);
  RUN_TEST(test_adc_failure_clears_after_valid_sequence);
  RUN_TEST(test_continuous_rail_fault_and_clear);
  RUN_TEST(test_shelf_rail_does_not_clear_active_fault);
  return UNITY_END();
}
