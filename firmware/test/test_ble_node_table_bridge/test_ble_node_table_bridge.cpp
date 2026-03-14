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
  TEST_ASSERT_EQUAL_UINT8(2, record_format);

  const uint8_t* record = page + 10;
  TEST_ASSERT_TRUE(len >= 10 + BleNodeTableBridge::kRecordBytesBle);
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
  const size_t records0 = (len0 - 10) / BleNodeTableBridge::kRecordBytesBle;
  TEST_ASSERT_EQUAL_UINT32(10, records0);

  const uint16_t snapshot_id = read_u16_le(page0);
  transport.set_node_table_request(snapshot_id, 1);
  TEST_ASSERT_TRUE(bridge.update_node_table(5000, table, transport));
  const size_t len1 = transport.node_table_len();
  const size_t records1 = (len1 - 10) / BleNodeTableBridge::kRecordBytesBle;
  TEST_ASSERT_EQUAL_UINT32(2, records1);
}

void test_stale_snapshot_fallback_freshness() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  const uint64_t self_id = 0xDDDDDDDDDDDDDDDDULL;
  table.init_self(self_id, 1000);
  table.update_self_position(1000, 2000, 0, 1000);

  const uint16_t stale_id = table.create_snapshot(1000);
  table.create_snapshot(2000);

  transport.set_node_table_request(stale_id, 0);
  TEST_ASSERT_TRUE(bridge.update_node_table(2000, table, transport));

  const uint8_t* page = transport.node_table_data();
  TEST_ASSERT_NOT_NULL(page);
  const uint16_t response_snapshot_id = read_u16_le(page);
  const uint16_t total_nodes = read_u16_le(page + 2);
  const size_t len = transport.node_table_len();
  const size_t record_count = (len - 10) / BleNodeTableBridge::kRecordBytesBle;

  TEST_ASSERT_FALSE(response_snapshot_id == stale_id);
  TEST_ASSERT_EQUAL_UINT16(1, total_nodes);
  TEST_ASSERT_EQUAL_UINT32(1, record_count);
}

/** S04 #465: 2s coalescing; first call refreshes prev (self only), add remote, next window emits one record. */
void test_subscription_batch_coalescing() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  const uint64_t self_id = 0x1111111111111111ULL;
  table.init_self(self_id, 1000);

  bridge.update_subscription_batch(1000, table, transport);
  TEST_ASSERT_EQUAL(0, transport.subscription_update_len());

  table.upsert_remote(0x2222222222222222ULL, true, 1000000, 2000000, 10, -65, 1, 1500);
  bridge.update_subscription_batch(1500, table, transport);
  TEST_ASSERT_EQUAL(0, transport.subscription_update_len());

  bridge.update_subscription_batch(3500, table, transport);
  TEST_ASSERT_TRUE(transport.subscription_update_len() >= 1 + BleNodeTableBridge::kRecordBytesBle);
  const uint8_t* batch = transport.subscription_update_data();
  TEST_ASSERT_NOT_NULL(batch);
  TEST_ASSERT_EQUAL_UINT8(1, batch[0]);
  TEST_ASSERT_EQUAL_UINT64(0x2222222222222222ULL, read_u64_le(batch + 1));
}

/** S04 #465: Payload count and length reflect actual packed records only (no bogus trailing). */
void test_subscription_batch_actual_packed_count() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x1111111111111111ULL, 1000);
  bridge.update_subscription_batch(1000, table, transport);
  table.upsert_remote(0x2222222222222222ULL, true, 1000000, 2000000, 10, -65, 1, 1500);
  table.upsert_remote(0x3333333333333333ULL, true, 2000000, 3000000, 5, -70, 2, 1600);
  bridge.update_subscription_batch(1500, table, transport);
  bridge.update_subscription_batch(3500, table, transport);

  const size_t len = transport.subscription_update_len();
  const uint8_t* batch = transport.subscription_update_data();
  TEST_ASSERT_NOT_NULL(batch);
  const uint8_t count = batch[0];
  TEST_ASSERT_EQUAL_UINT8(2, count);
  TEST_ASSERT_EQUAL(len, 1 + count * BleNodeTableBridge::kRecordBytesBle);
}

/** S04 #465: Overflow beyond batch cap is retained and emitted in next window. */
void test_subscription_batch_overflow_retention() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x1111111111111111ULL, 1000);
  bridge.update_subscription_batch(1000, table, transport);
  for (size_t i = 0; i < 7; ++i) {
    table.upsert_remote(0x2000000000000000ULL + i, true,
                        static_cast<int32_t>(1000000 + i), 2000000, 10, -65, 1,
                        static_cast<uint32_t>(1500 + i));
  }
  bridge.update_subscription_batch(1500, table, transport);
  bridge.update_subscription_batch(3500, table, transport);

  const uint8_t* batch1 = transport.subscription_update_data();
  TEST_ASSERT_NOT_NULL(batch1);
  TEST_ASSERT_EQUAL_UINT8(5, batch1[0]);
  TEST_ASSERT_EQUAL(1 + 5 * BleNodeTableBridge::kRecordBytesBle, transport.subscription_update_len());

  bridge.update_subscription_batch(5500, table, transport);
  const uint8_t* batch2 = transport.subscription_update_data();
  TEST_ASSERT_NOT_NULL(batch2);
  TEST_ASSERT_EQUAL_UINT8(2, batch2[0]);
  TEST_ASSERT_EQUAL(1 + 2 * BleNodeTableBridge::kRecordBytesBle, transport.subscription_update_len());
}

/** S04 #465: Change in an exported BLE field (e.g. last_rx_rssi) triggers subscription update. */
void test_subscription_exported_field_change_triggers_update() {
  MockBleTransport transport;
  BleNodeTableBridge bridge;
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x1111111111111111ULL, 1000);
  bridge.update_subscription_batch(1000, table, transport);
  table.upsert_remote(0x2222222222222222ULL, true, 1000000, 2000000, 10, -65, 1, 1500);
  bridge.update_subscription_batch(1500, table, transport);
  bridge.update_subscription_batch(3500, table, transport);
  TEST_ASSERT_TRUE(transport.subscription_update_len() >= 1 + BleNodeTableBridge::kRecordBytesBle);
  TEST_ASSERT_EQUAL_UINT8(1, transport.subscription_update_data()[0]);
  table.upsert_remote(0x2222222222222222ULL, true, 1000000, 2000000, 10, -70, 2, 4500);
  bridge.update_subscription_batch(4500, table, transport);
  bridge.update_subscription_batch(6500, table, transport);
  TEST_ASSERT_TRUE(transport.subscription_update_len() >= 1 + BleNodeTableBridge::kRecordBytesBle);
  TEST_ASSERT_EQUAL_UINT8(1, transport.subscription_update_data()[0]);
  TEST_ASSERT_EQUAL_UINT64(0x2222222222222222ULL, read_u64_le(transport.subscription_update_data() + 1));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_device_info_payload);
  RUN_TEST(test_snapshot_header_and_record);
  RUN_TEST(test_paging_overflow);
  RUN_TEST(test_stale_snapshot_fallback_freshness);
  RUN_TEST(test_subscription_batch_coalescing);
  RUN_TEST(test_subscription_batch_actual_packed_count);
  RUN_TEST(test_subscription_batch_overflow_retention);
  RUN_TEST(test_subscription_exported_field_change_triggers_update);
  return UNITY_END();
}
