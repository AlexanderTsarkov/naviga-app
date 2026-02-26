#include <unity.h>

#include <cstdint>

#include "../../protocol/ble_node_table_bridge.h"
#include "../../protocol/ble_node_table_bridge.cpp"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/alive_codec.h"
#include "../../src/domain/beacon_logic.h"
#include "../../src/domain/beacon_logic.cpp"
#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"
#include "../../lib/NavigaCore/include/naviga/hal/mocks/mock_ble_transport.h"
#include "../../lib/NavigaCore/src/mocks/mock_ble_transport.cpp"

using naviga::MockBleTransport;
using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::protocol::BleNodeTableBridge;
using naviga::protocol::DeviceInfoModel;
using naviga::protocol::GeoBeaconFields;

namespace {

constexpr size_t kPageHeaderBytes = 10;

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

bool buffer_contains_node_id(const uint8_t* buffer, size_t len, uint64_t node_id) {
  if (!buffer) {
    return false;
  }
  for (size_t offset = 0; offset + NodeTable::kRecordBytes <= len; offset += NodeTable::kRecordBytes) {
    if (read_u64_le(buffer + offset) == node_id) {
      return true;
    }
  }
  return false;
}

bool page_contains_node_id(const uint8_t* page, size_t len, uint64_t node_id) {
  if (!page || len <= kPageHeaderBytes) {
    return false;
  }
  const size_t records_len = len - kPageHeaderBytes;
  return buffer_contains_node_id(page + kPageHeaderBytes, records_len, node_id);
}

} // namespace

void test_e2e_beacon_to_ble_bridge() {
  BeaconLogic tx_logic;
  BeaconLogic rx_logic;
  tx_logic.set_min_interval_ms(1000);

  NodeTable table_b;
  table_b.set_expected_interval_s(10);
  // NodeID48: upper 16 bits = 0x0000 (domain invariant).
  const uint64_t node_b_id = 0x0000B0B0B0B0B0B0ULL;
  table_b.init_self(node_b_id, 1000);

  GeoBeaconFields fields{};
  // NodeID48: upper 16 bits = 0x0000 so round-trip through codec is lossless.
  fields.node_id = 0x0000030405060708ULL;
  fields.pos_valid = 1;
  fields.lat_deg = 55.7558;   // Moscow — canonical example from beacon_payload_encoding_v0 §5.1
  fields.lon_deg = 37.6173;

  uint8_t payload[naviga::protocol::kGeoBeaconFrameSize] = {};
  size_t payload_len = 0;
  TEST_ASSERT_TRUE(tx_logic.build_tx(1000, fields, payload, sizeof(payload), &payload_len));
  TEST_ASSERT_EQUAL_UINT32(naviga::protocol::kGeoBeaconFrameSize, payload_len);

  TEST_ASSERT_TRUE(rx_logic.on_rx(1010, payload, payload_len, -72, table_b));

  const uint16_t snapshot_id = table_b.create_snapshot(1020);
  uint8_t snapshot_buf[NodeTable::kRecordBytes * NodeTable::kDefaultPageSize] = {};
  const size_t snapshot_bytes = table_b.get_snapshot_page(snapshot_id,
                                                         0,
                                                         NodeTable::kDefaultPageSize,
                                                         snapshot_buf,
                                                         sizeof(snapshot_buf));
  TEST_ASSERT_TRUE(snapshot_bytes >= NodeTable::kRecordBytes);
  TEST_ASSERT_TRUE(buffer_contains_node_id(snapshot_buf, snapshot_bytes, fields.node_id));

  BleNodeTableBridge bridge;
  MockBleTransport transport;
  DeviceInfoModel info{};
  info.format_ver = 1;
  info.ble_schema_ver = 1;
  info.node_id = node_b_id;
  info.short_id = NodeTable::compute_short_id(node_b_id);
  info.device_type = 1;
  info.firmware_version = "ootb-e2e-test";
  info.radio_module_model = "E220";
  info.band_id = 433;
  info.channel_min = 0;
  info.channel_max = 255;
  info.network_mode = 0;
  info.channel_id = 1;
  info.public_channel_id = 1;
  info.capabilities = 0;

  transport.set_node_table_request(0, 0);
  TEST_ASSERT_TRUE(bridge.update_all(1020, info, table_b, transport));
  TEST_ASSERT_TRUE(transport.device_info_len() > 0);

  const uint8_t* page0 = transport.node_table_data();
  TEST_ASSERT_NOT_NULL(page0);
  const size_t page_len = transport.node_table_len();
  TEST_ASSERT_TRUE(page_len > kPageHeaderBytes);

  const uint16_t total_nodes = read_u16_le(page0 + 2);
  const uint8_t record_format = page0[9];
  TEST_ASSERT_TRUE(total_nodes >= 1);
  TEST_ASSERT_EQUAL_UINT8(BleNodeTableBridge::kRecordFormatVer, record_format);
  TEST_ASSERT_TRUE(page_contains_node_id(page0, page_len, fields.node_id));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_e2e_beacon_to_ble_bridge);
  return UNITY_END();
}
