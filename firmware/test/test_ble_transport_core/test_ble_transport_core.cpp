#include <unity.h>

#include <cstddef>
#include <cstdint>

#include "platform/ble_transport_core.h"

using naviga::BleTransportCore;

void test_device_info_store_and_truncate() {
  BleTransportCore core;
  uint8_t data[BleTransportCore::kMaxDeviceInfoLen + 5] = {0};
  for (size_t i = 0; i < sizeof(data); ++i) {
    data[i] = static_cast<uint8_t>(i & 0xFF);
  }

  core.set_device_info(data, sizeof(data));
  TEST_ASSERT_EQUAL_UINT32(BleTransportCore::kMaxDeviceInfoLen, core.device_info_len());
  const uint8_t* out = core.device_info_data();
  TEST_ASSERT_NOT_NULL(out);
  TEST_ASSERT_EQUAL_UINT8(0, out[0]);
  TEST_ASSERT_EQUAL_UINT8(1, out[1]);
  TEST_ASSERT_EQUAL_UINT8(255, out[255]);
}

void test_node_table_page_bounds_and_truncate() {
  BleTransportCore core;
  uint8_t page[BleTransportCore::kMaxPageLen + 3] = {0};
  for (size_t i = 0; i < sizeof(page); ++i) {
    page[i] = static_cast<uint8_t>((i + 7) & 0xFF);
  }

  TEST_ASSERT_FALSE(core.set_node_table_page(4, page, sizeof(page)));
  TEST_ASSERT_EQUAL_UINT32(0, core.page_len(4));

  TEST_ASSERT_TRUE(core.set_node_table_page(2, page, sizeof(page)));
  TEST_ASSERT_EQUAL_UINT32(BleTransportCore::kMaxPageLen, core.page_len(2));
  const uint8_t* out = core.page_data(2);
  TEST_ASSERT_NOT_NULL(out);
  TEST_ASSERT_EQUAL_UINT8(7, out[0]);
  TEST_ASSERT_EQUAL_UINT8(8, out[1]);
}

void test_getters_exact_bytes() {
  BleTransportCore core;
  const uint8_t dev_info[] = {0xAA, 0xBB, 0xCC, 0xDD};
  core.set_device_info(dev_info, sizeof(dev_info));
  TEST_ASSERT_EQUAL_UINT32(sizeof(dev_info), core.device_info_len());
  const uint8_t* out = core.device_info_data();
  TEST_ASSERT_NOT_NULL(out);
  for (size_t i = 0; i < sizeof(dev_info); ++i) {
    TEST_ASSERT_EQUAL_UINT8(dev_info[i], out[i]);
  }

  const uint8_t page[] = {1, 2, 3, 4, 5};
  core.set_node_table_page(0, page, sizeof(page));
  TEST_ASSERT_EQUAL_UINT32(sizeof(page), core.page_len(0));
  const uint8_t* page_out = core.page_data(0);
  TEST_ASSERT_NOT_NULL(page_out);
  for (size_t i = 0; i < sizeof(page); ++i) {
    TEST_ASSERT_EQUAL_UINT8(page[i], page_out[i]);
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_device_info_store_and_truncate);
  RUN_TEST(test_node_table_page_bounds_and_truncate);
  RUN_TEST(test_getters_exact_bytes);
  return UNITY_END();
}
