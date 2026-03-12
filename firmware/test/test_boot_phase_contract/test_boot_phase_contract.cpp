/**
 * #423: Boot phase contract tests (Phase A boot_config_result).
 * Verifies RadioBootConfigResult enum ordering so "worst of" (GNSS, radio) is well-defined.
 * module_boot_config_v0 §4: Ok / Repaired / RepairFailed; RepairFailed must not brick.
 */
#include <unity.h>

#include <algorithm>
#include <cstdint>

#include "naviga/hal/interfaces.h"

using naviga::RadioBootConfigResult;

namespace {

/** Canonical "worst of" for Phase A combined boot_config_result (#393). */
RadioBootConfigResult worst_of(RadioBootConfigResult a, RadioBootConfigResult b) {
  const auto u = static_cast<uint8_t>(a);
  const auto v = static_cast<uint8_t>(b);
  return static_cast<RadioBootConfigResult>(std::max(u, v));
}

}  // namespace

void test_boot_config_result_enum_ordering() {
  TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(RadioBootConfigResult::Ok));
  TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(RadioBootConfigResult::Repaired));
  TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(RadioBootConfigResult::RepairFailed));
}

void test_worst_of_ok_ok_is_ok() {
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::Ok),
                   static_cast<int>(worst_of(RadioBootConfigResult::Ok, RadioBootConfigResult::Ok)));
}

void test_worst_of_any_repair_failed_is_repair_failed() {
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::RepairFailed),
                   static_cast<int>(worst_of(RadioBootConfigResult::RepairFailed, RadioBootConfigResult::Ok)));
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::RepairFailed),
                   static_cast<int>(worst_of(RadioBootConfigResult::Ok, RadioBootConfigResult::RepairFailed)));
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::RepairFailed),
                   static_cast<int>(worst_of(RadioBootConfigResult::Repaired, RadioBootConfigResult::RepairFailed)));
}

void test_worst_of_ok_repaired_is_repaired() {
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::Repaired),
                   static_cast<int>(worst_of(RadioBootConfigResult::Ok, RadioBootConfigResult::Repaired)));
  TEST_ASSERT_EQUAL(static_cast<int>(RadioBootConfigResult::Repaired),
                   static_cast<int>(worst_of(RadioBootConfigResult::Repaired, RadioBootConfigResult::Ok)));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_boot_config_result_enum_ordering);
  RUN_TEST(test_worst_of_ok_ok_is_ok);
  RUN_TEST(test_worst_of_any_repair_failed_is_repair_failed);
  RUN_TEST(test_worst_of_ok_repaired_is_repaired);
  return UNITY_END();
}
