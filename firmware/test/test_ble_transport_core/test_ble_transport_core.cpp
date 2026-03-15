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

void test_node_table_response_truncate() {
  BleTransportCore core;
  uint8_t page[BleTransportCore::kMaxPageLen + 3] = {0};
  for (size_t i = 0; i < sizeof(page); ++i) {
    page[i] = static_cast<uint8_t>((i + 7) & 0xFF);
  }

  core.set_node_table_response(page, sizeof(page));
  TEST_ASSERT_EQUAL_UINT32(BleTransportCore::kMaxPageLen, core.node_table_response_len());
  const uint8_t* out = core.node_table_response_data();
  TEST_ASSERT_NOT_NULL(out);
  TEST_ASSERT_EQUAL_UINT8(7, out[0]);
  TEST_ASSERT_EQUAL_UINT8(8, out[1]);
}

void test_node_table_request_store() {
  BleTransportCore core;
  uint16_t snapshot_id = 0;
  uint16_t page_index = 0;
  TEST_ASSERT_FALSE(core.get_node_table_request(&snapshot_id, &page_index));

  core.set_node_table_request(42, 3);
  TEST_ASSERT_TRUE(core.get_node_table_request(&snapshot_id, &page_index));
  TEST_ASSERT_EQUAL_UINT16(42, snapshot_id);
  TEST_ASSERT_EQUAL_UINT16(3, page_index);
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
  core.set_node_table_response(page, sizeof(page));
  TEST_ASSERT_EQUAL_UINT32(sizeof(page), core.node_table_response_len());
  const uint8_t* page_out = core.node_table_response_data();
  TEST_ASSERT_NOT_NULL(page_out);
  for (size_t i = 0; i < sizeof(page); ++i) {
    TEST_ASSERT_EQUAL_UINT8(page[i], page_out[i]);
  }
}

// S04 #466: node_name read value and write request buffers.
void test_node_name_value_and_write_request() {
  BleTransportCore core;
  const uint8_t read_payload[] = {3, 'A', 'B', 'C'};
  core.set_node_name_value(read_payload, sizeof(read_payload));
  TEST_ASSERT_EQUAL_UINT32(sizeof(read_payload), core.node_name_value_len());
  const uint8_t* rd = core.node_name_value_data();
  TEST_ASSERT_NOT_NULL(rd);
  TEST_ASSERT_EQUAL_UINT8(3, rd[0]);
  TEST_ASSERT_EQUAL_UINT8('A', rd[1]);

  TEST_ASSERT_FALSE(core.has_node_name_write_request());
  const uint8_t write_payload[] = {2, 'X', 'Y'};
  core.set_node_name_write_request(write_payload, sizeof(write_payload));
  TEST_ASSERT_TRUE(core.has_node_name_write_request());
  TEST_ASSERT_EQUAL_UINT32(sizeof(write_payload), core.node_name_write_request_len());
  const uint8_t* wr = core.node_name_write_request_data();
  TEST_ASSERT_NOT_NULL(wr);
  TEST_ASSERT_EQUAL_UINT8(2, wr[0]);
  TEST_ASSERT_EQUAL_UINT8('X', wr[1]);

  core.clear_node_name_write_request();
  TEST_ASSERT_FALSE(core.has_node_name_write_request());
  TEST_ASSERT_EQUAL_UINT32(0, core.node_name_write_request_len());
}

// S04 #467: Profiles list and profile read request/response.
void test_profiles_list_and_read() {
  BleTransportCore core;
  const uint8_t list[] = {1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0};
  core.set_profiles_list(list, sizeof(list));
  TEST_ASSERT_EQUAL_UINT32(sizeof(list), core.profiles_list_len());
  const uint8_t* list_out = core.profiles_list_data();
  TEST_ASSERT_NOT_NULL(list_out);
  TEST_ASSERT_EQUAL_UINT8(1, list_out[0]);
  TEST_ASSERT_EQUAL_UINT8(3, list_out[5]);

  TEST_ASSERT_FALSE(core.has_profile_read_request());
  core.set_profile_read_request(0, 0);
  uint8_t type = 0xFF;
  uint32_t id = 0xFFFFFFFFu;
  TEST_ASSERT_TRUE(core.get_profile_read_request(&type, &id));
  TEST_ASSERT_EQUAL_UINT8(0, type);
  TEST_ASSERT_EQUAL_UINT32(0, id);

  const uint8_t resp[] = {0, 0, 0, 0, 0, 1, 2, 0, 0};
  core.set_profile_read_response(resp, sizeof(resp));
  TEST_ASSERT_EQUAL_UINT32(sizeof(resp), core.profile_read_response_len());
  const uint8_t* resp_out = core.profile_read_response_data();
  TEST_ASSERT_NOT_NULL(resp_out);
  TEST_ASSERT_EQUAL_UINT8(0, resp_out[0]);

  core.clear_profile_read_request();
  TEST_ASSERT_FALSE(core.has_profile_read_request());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_device_info_store_and_truncate);
  RUN_TEST(test_node_table_response_truncate);
  RUN_TEST(test_node_table_request_store);
  RUN_TEST(test_getters_exact_bytes);
  RUN_TEST(test_node_name_value_and_write_request);
  RUN_TEST(test_profiles_list_and_read);
  return UNITY_END();
}
