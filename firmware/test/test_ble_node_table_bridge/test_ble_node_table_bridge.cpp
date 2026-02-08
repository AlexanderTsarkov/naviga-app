#include <unity.h>

#include <cstdint>

#include "../../protocol/ble_node_table_bridge.h"
#include "../../protocol/ble_node_table_bridge.cpp"
#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"
#include "../../lib/NavigaCore/include/naviga/hal/mocks/mock_ble_transport.h"
#include "../../lib/NavigaCore/src/mocks/mock_ble_transport.cpp"

using naviga::MockBleTransport;
using naviga::domain::NodeTable;
using naviga::protocol::BleNodeTableBridge;
using naviga::protocol::DeviceInfoModel;

namespace {

uint16_t read_u16_le(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

uint64_t read_u64_le(const uint8_t* data) {
  uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= (static_cast<uint64_t>(data[i]) << (8 * i));
  }
  return value;
}

} // namespace

void test_device_info_payload() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  DeviceInfoModel model{};
  model.format_ver = 1;
  model.ble_schema_ver = 1;
  model.node_id = 0x0102030405060708ULL;
  model.short_id = 0xBEEF;
  model.device_type = 1;
  model.firmware_version = "fw";
  model.radio_module_model = "E220";
  model.band_id = 433;
  model.channel_min = 0;
  model.channel_max = 255;
  model.network_mode = 0;
  model.channel_id = 1;
  model.capabilities = 0x03;

  TEST_ASSERT_TRUE(bridge.update_device_info(model, transport));
  TEST_ASSERT_TRUE(transport.device_info_len() > 0);
  const uint8_t* data = transport.device_info();
  TEST_ASSERT_EQUAL_UINT8(1, data[0]);
  TEST_ASSERT_EQUAL_UINT8(1, data[1]);
}

void test_snapshot_header_and_record() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  const uint64_t self_id = 0x1111111111111111ULL;
  table.init_self(self_id, 1000);
  table.update_self_position(123, 456, 5, 1000);

  transport.set_node_table_request(0, 0);
  TEST_ASSERT_TRUE(bridge.update_node_table(1000, table, transport));
  const size_t len = transport.node_table_len();
  TEST_ASSERT_TRUE(len > 0);
  const uint8_t* page = transport.node_table_data();
  TEST_ASSERT_NOT_NULL(page);

  const uint16_t snapshot_id = read_u16_le(page);
  const uint16_t total_nodes = read_u16_le(page + 2);
  const uint16_t page_index = read_u16_le(page + 4);
  const uint8_t page_size = page[6];
  const uint16_t page_count = read_u16_le(page + 7);
  const uint8_t record_format = page[9];

  TEST_ASSERT_TRUE(snapshot_id != 0);
  TEST_ASSERT_EQUAL_UINT16(1, total_nodes);
  TEST_ASSERT_EQUAL_UINT16(0, page_index);
  TEST_ASSERT_EQUAL_UINT8(10, page_size);
  TEST_ASSERT_EQUAL_UINT16(1, page_count);
  TEST_ASSERT_EQUAL_UINT8(1, record_format);

  const uint8_t* record = page + 10;
  TEST_ASSERT_EQUAL_UINT64(self_id, read_u64_le(record));
}

void test_paging_overflow() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0xAAAAAAAAAAAAAAAAULL, 0);

  const uint64_t base_id = 0x1000000000000000ULL;
  for (size_t i = 0; i < 11; ++i) {
    table.upsert_remote(base_id + i, false, 0, 0, 0, -70, 1, static_cast<uint32_t>(i));
  }

  transport.set_node_table_request(0, 0);
  TEST_ASSERT_TRUE(bridge.update_node_table(5000, table, transport));

  const uint8_t* page0 = transport.node_table_data();
  TEST_ASSERT_NOT_NULL(page0);

  const uint16_t page_count = read_u16_le(page0 + 7);
  TEST_ASSERT_EQUAL_UINT16(2, page_count);

  const size_t len0 = transport.node_table_len();
  const size_t records0 = (len0 - 10) / NodeTable::kRecordBytes;
  TEST_ASSERT_EQUAL_UINT32(10, records0);

  const uint16_t snapshot_id = read_u16_le(page0);
  transport.set_node_table_request(snapshot_id, 1);
  TEST_ASSERT_TRUE(bridge.update_node_table(5000, table, transport));
  const size_t len1 = transport.node_table_len();
  const size_t records1 = (len1 - 10) / NodeTable::kRecordBytes;
  TEST_ASSERT_EQUAL_UINT32(2, records1);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_device_info_payload);
  RUN_TEST(test_snapshot_header_and_record);
  RUN_TEST(test_paging_overflow);
  return UNITY_END();
}
