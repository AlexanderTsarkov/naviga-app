/**
 * Unit tests for #289: role profile record and OOTB defaults.
 * Tests get_ootb_role_profile (no NVS); fallback behavior is tested on device.
 */
#include <unity.h>

#include <cstdint>

#include "../../src/platform/naviga_storage.h"
#include "../../src/platform/role_profile_ootb.cpp"

using naviga::RoleProfileRecord;
using naviga::get_ootb_role_profile;

namespace {

void expect_person(const RoleProfileRecord& r) {
  TEST_ASSERT_EQUAL_UINT16(18, r.min_interval_sec);
  TEST_ASSERT_EQUAL_UINT8(9, r.max_silence_10s);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, r.min_displacement_m);
}

void expect_dog(const RoleProfileRecord& r) {
  TEST_ASSERT_EQUAL_UINT16(9, r.min_interval_sec);
  TEST_ASSERT_EQUAL_UINT8(3, r.max_silence_10s);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 15.0f, r.min_displacement_m);
}

void expect_infra(const RoleProfileRecord& r) {
  TEST_ASSERT_EQUAL_UINT16(360, r.min_interval_sec);
  TEST_ASSERT_EQUAL_UINT8(255, r.max_silence_10s);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, r.min_displacement_m);
}

}  // namespace

void test_ootb_role_0_person() {
  RoleProfileRecord out{};
  get_ootb_role_profile(0, &out);
  expect_person(out);
}

void test_ootb_role_1_dog() {
  RoleProfileRecord out{};
  get_ootb_role_profile(1, &out);
  expect_dog(out);
}

void test_ootb_role_2_infra() {
  RoleProfileRecord out{};
  get_ootb_role_profile(2, &out);
  expect_infra(out);
}

void test_ootb_role_invalid_fallback_default() {
  RoleProfileRecord out{};
  get_ootb_role_profile(99, &out);
  expect_person(out);
}

void test_ootb_null_out_no_crash() {
  get_ootb_role_profile(0, nullptr);
  get_ootb_role_profile(1, nullptr);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_ootb_role_0_person);
  RUN_TEST(test_ootb_role_1_dog);
  RUN_TEST(test_ootb_role_2_infra);
  RUN_TEST(test_ootb_role_invalid_fallback_default);
  RUN_TEST(test_ootb_null_out_no_crash);
  return UNITY_END();
}
