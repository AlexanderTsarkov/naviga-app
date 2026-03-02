#include <unity.h>

#include <cstdint>

// Header-only — no .cpp to include.
#include "../../lib/NavigaCore/include/naviga/hal/radio_preset.h"

using namespace naviga;

// ── normalize_air_rate ────────────────────────────────────────────────────────

void test_normalize_air_rate_0_clamped_to_2() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(0, &out);
  TEST_ASSERT_TRUE(clamped);
  TEST_ASSERT_EQUAL_UINT8(2, out);
}

void test_normalize_air_rate_1_clamped_to_2() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(1, &out);
  TEST_ASSERT_TRUE(clamped);
  TEST_ASSERT_EQUAL_UINT8(2, out);
}

void test_normalize_air_rate_2_unchanged() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(2, &out);
  TEST_ASSERT_FALSE(clamped);
  TEST_ASSERT_EQUAL_UINT8(2, out);
}

void test_normalize_air_rate_3_unchanged() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(3, &out);
  TEST_ASSERT_FALSE(clamped);
  TEST_ASSERT_EQUAL_UINT8(3, out);
}

void test_normalize_air_rate_4_unchanged() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(4, &out);
  TEST_ASSERT_FALSE(clamped);
  TEST_ASSERT_EQUAL_UINT8(4, out);
}

void test_normalize_air_rate_5_unchanged() {
  uint8_t out = 99;
  bool clamped = normalize_air_rate(5, &out);
  TEST_ASSERT_FALSE(clamped);
  TEST_ASSERT_EQUAL_UINT8(5, out);
}

// ── verify_preset_readback ────────────────────────────────────────────────────

void test_verify_preset_readback_all_match() {
  RadioPreset norm{/*.air_rate=*/2, /*.channel=*/1, /*.tx_power_dbm=*/30};
  RadioPresetVerifyResult r = verify_preset_readback(norm, 2, 1);
  TEST_ASSERT_TRUE(r.air_rate_ok);
  TEST_ASSERT_TRUE(r.channel_ok);
  TEST_ASSERT_TRUE(r.all_ok());
}

void test_verify_preset_readback_air_rate_mismatch() {
  RadioPreset norm{/*.air_rate=*/3, /*.channel=*/1, /*.tx_power_dbm=*/30};
  // Module returned 2 instead of 3 (hypothetical)
  RadioPresetVerifyResult r = verify_preset_readback(norm, 2, 1);
  TEST_ASSERT_FALSE(r.air_rate_ok);
  TEST_ASSERT_TRUE(r.channel_ok);
  TEST_ASSERT_FALSE(r.all_ok());
}

void test_verify_preset_readback_channel_mismatch() {
  RadioPreset norm{/*.air_rate=*/2, /*.channel=*/2, /*.tx_power_dbm=*/30};
  RadioPresetVerifyResult r = verify_preset_readback(norm, 2, 1);
  TEST_ASSERT_TRUE(r.air_rate_ok);
  TEST_ASSERT_FALSE(r.channel_ok);
  TEST_ASSERT_FALSE(r.all_ok());
}

void test_verify_preset_readback_both_mismatch() {
  RadioPreset norm{/*.air_rate=*/4, /*.channel=*/3, /*.tx_power_dbm=*/30};
  RadioPresetVerifyResult r = verify_preset_readback(norm, 2, 1);
  TEST_ASSERT_FALSE(r.air_rate_ok);
  TEST_ASSERT_FALSE(r.channel_ok);
  TEST_ASSERT_FALSE(r.all_ok());
}

// ── get_radio_preset ──────────────────────────────────────────────────────────

void test_get_radio_preset_default_air_rate_2() {
  RadioPreset p = get_radio_preset(RadioPresetId::Default);
  TEST_ASSERT_EQUAL_UINT8(2, p.air_rate);
  TEST_ASSERT_EQUAL_UINT8(1, p.channel);
}

void test_get_radio_preset_fast_air_rate_3() {
  RadioPreset p = get_radio_preset(RadioPresetId::Fast);
  TEST_ASSERT_EQUAL_UINT8(3, p.air_rate);
  TEST_ASSERT_EQUAL_UINT8(1, p.channel);
}

void test_get_radio_preset_longrange_air_rate_2() {
  RadioPreset p = get_radio_preset(RadioPresetId::LongRange);
  TEST_ASSERT_EQUAL_UINT8(2, p.air_rate);
  TEST_ASSERT_EQUAL_UINT8(1, p.channel);
}

// All defined presets have air_rate >= kMinAirRate (no clamp needed for canonical presets).
void test_all_canonical_presets_no_clamp_needed() {
  const RadioPresetId ids[] = {
    RadioPresetId::Default,
    RadioPresetId::Fast,
    RadioPresetId::LongRange,
  };
  for (auto id : ids) {
    RadioPreset p = get_radio_preset(id);
    uint8_t norm = 0;
    bool clamped = normalize_air_rate(p.air_rate, &norm);
    TEST_ASSERT_FALSE_MESSAGE(clamped, "canonical preset should not require clamping");
    TEST_ASSERT_EQUAL_UINT8(p.air_rate, norm);
  }
}

// ── main ──────────────────────────────────────────────────────────────────────

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_normalize_air_rate_0_clamped_to_2);
  RUN_TEST(test_normalize_air_rate_1_clamped_to_2);
  RUN_TEST(test_normalize_air_rate_2_unchanged);
  RUN_TEST(test_normalize_air_rate_3_unchanged);
  RUN_TEST(test_normalize_air_rate_4_unchanged);
  RUN_TEST(test_normalize_air_rate_5_unchanged);

  RUN_TEST(test_verify_preset_readback_all_match);
  RUN_TEST(test_verify_preset_readback_air_rate_mismatch);
  RUN_TEST(test_verify_preset_readback_channel_mismatch);
  RUN_TEST(test_verify_preset_readback_both_mismatch);

  RUN_TEST(test_get_radio_preset_default_air_rate_2);
  RUN_TEST(test_get_radio_preset_fast_air_rate_3);
  RUN_TEST(test_get_radio_preset_longrange_air_rate_2);
  RUN_TEST(test_all_canonical_presets_no_clamp_needed);

  return UNITY_END();
}
