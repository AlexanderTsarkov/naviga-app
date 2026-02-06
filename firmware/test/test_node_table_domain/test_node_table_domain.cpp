#include <unity.h>

#include <array>
#include <cstdint>

#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"

using naviga::domain::NodeTable;

namespace {

uint16_t read_u16_le(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

uint32_t read_u32_le(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

uint64_t read_u64_le(const uint8_t* data) {
  uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= (static_cast<uint64_t>(data[i]) << (8 * i));
  }
  return value;
}

struct RecordView {
  uint64_t node_id = 0;
  uint16_t short_id = 0;
  uint8_t flags = 0;
  uint16_t last_seen_age_s = 0;
  int32_t lat_e7 = 0;
  int32_t lon_e7 = 0;
  uint16_t pos_age_s = 0;
  int8_t last_rx_rssi = 0;
  uint16_t last_seq = 0;
};

RecordView decode_record(const uint8_t* data) {
  RecordView out{};
  out.node_id = read_u64_le(data);
  out.short_id = read_u16_le(data + 8);
  out.flags = data[10];
  out.last_seen_age_s = read_u16_le(data + 11);
  out.lat_e7 = static_cast<int32_t>(read_u32_le(data + 13));
  out.lon_e7 = static_cast<int32_t>(read_u32_le(data + 17));
  out.pos_age_s = read_u16_le(data + 21);
  out.last_rx_rssi = static_cast<int8_t>(data[23]);
  out.last_seq = read_u16_le(data + 24);
  return out;
}

bool buffer_contains_node_id(const uint8_t* data, size_t bytes, uint64_t node_id) {
  const size_t count = bytes / NodeTable::kRecordBytes;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* record = data + i * NodeTable::kRecordBytes;
    if (read_u64_le(record) == node_id) {
      return true;
    }
  }
  return false;
}

bool find_record(const uint8_t* data, size_t bytes, uint64_t node_id, RecordView* out) {
  const size_t count = bytes / NodeTable::kRecordBytes;
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* record = data + i * NodeTable::kRecordBytes;
    if (read_u64_le(record) == node_id) {
      if (out) {
        *out = decode_record(record);
      }
      return true;
    }
  }
  return false;
}

} // namespace

void test_self_init_and_serialization() {
  NodeTable table;
  table.set_expected_interval_s(10);
  const uint64_t self_id = 0x0102030405060708ULL;
  table.init_self(self_id, 1000);

  uint8_t buffer[NodeTable::kRecordBytes * 2] = {};
  const size_t bytes = table.get_page(1000, 0, 10, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kRecordBytes, bytes);

  const RecordView record = decode_record(buffer);
  TEST_ASSERT_EQUAL_UINT64(self_id, record.node_id);
  TEST_ASSERT_EQUAL_UINT16(NodeTable::compute_short_id(self_id), record.short_id);
  TEST_ASSERT_TRUE((record.flags & 0x01) != 0);
  TEST_ASSERT_TRUE((record.flags & 0x02) == 0);
  TEST_ASSERT_TRUE((record.flags & 0x04) == 0);
  TEST_ASSERT_EQUAL_UINT16(0, record.last_seen_age_s);
  TEST_ASSERT_EQUAL_INT32(0, record.lat_e7);
  TEST_ASSERT_EQUAL_INT32(0, record.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(0, record.pos_age_s);
  TEST_ASSERT_EQUAL_INT8(0, record.last_rx_rssi);
  TEST_ASSERT_EQUAL_UINT16(0, record.last_seq);
}

void test_remote_upsert_and_fields() {
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x1111111111111111ULL, 100);

  const uint64_t remote_id = 0x2222222222222222ULL;
  TEST_ASSERT_TRUE(table.upsert_remote(remote_id, true, 123, -456, 7, -30, 42, 2000));

  uint8_t buffer[NodeTable::kRecordBytes * 4] = {};
  const size_t bytes = table.get_page(2000, 0, 10, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kRecordBytes * 2, bytes);

  RecordView record{};
  TEST_ASSERT_TRUE(find_record(buffer, bytes, remote_id, &record));
  TEST_ASSERT_EQUAL_UINT16(NodeTable::compute_short_id(remote_id), record.short_id);
  TEST_ASSERT_TRUE((record.flags & 0x01) == 0);
  TEST_ASSERT_TRUE((record.flags & 0x02) != 0);
  TEST_ASSERT_TRUE((record.flags & 0x04) == 0);
  TEST_ASSERT_EQUAL_UINT16(0, record.last_seen_age_s);
  TEST_ASSERT_EQUAL_INT32(123, record.lat_e7);
  TEST_ASSERT_EQUAL_INT32(-456, record.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(7, record.pos_age_s);
  TEST_ASSERT_EQUAL_INT8(-30, record.last_rx_rssi);
  TEST_ASSERT_EQUAL_UINT16(42, record.last_seq);
}

void test_grey_transition() {
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x0101010101010101ULL, 0);
  const uint64_t remote_id = 0x0202020202020202ULL;
  TEST_ASSERT_TRUE(table.upsert_remote(remote_id, false, 0, 0, 0, -80, 1, 0));

  uint8_t buffer[NodeTable::kRecordBytes * 4] = {};
  const size_t bytes = table.get_page(14000, 0, 10, buffer, sizeof(buffer));
  RecordView record{};
  TEST_ASSERT_TRUE(find_record(buffer, bytes, remote_id, &record));
  TEST_ASSERT_TRUE((record.flags & 0x04) != 0);
}

void test_eviction_oldest_grey() {
  NodeTable table;
  table.set_expected_interval_s(1);
  table.init_self(0xAAAAAAAAAAAAAAAAULL, 0);

  const uint64_t base_id = 0x1000000000000000ULL;
  for (size_t i = 0; i < NodeTable::kMaxNodes - 1; ++i) {
    TEST_ASSERT_TRUE(table.upsert_remote(base_id + i, false, 0, 0, 0, -70, 1, static_cast<uint32_t>(i)));
  }

  const uint64_t evicted_id = base_id + 0;
  const uint64_t new_id = 0x2000000000000000ULL;
  TEST_ASSERT_TRUE(table.upsert_remote(new_id, false, 0, 0, 0, -60, 2, 5000));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kMaxNodes, table.size());

  const uint16_t snapshot_id = table.create_snapshot(5000);
  uint8_t buffer[NodeTable::kRecordBytes * NodeTable::kMaxNodes] = {};
  const size_t bytes = table.get_snapshot_page(snapshot_id, 0, NodeTable::kMaxNodes, buffer,
                                               sizeof(buffer));
  TEST_ASSERT_TRUE(buffer_contains_node_id(buffer, bytes, new_id));
  TEST_ASSERT_FALSE(buffer_contains_node_id(buffer, bytes, evicted_id));
}

void test_collision_flagging() {
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0x0100000000000000ULL, 0);

  std::array<uint64_t, 65536> seen{};
  uint64_t id_a = 0;
  uint64_t id_b = 0;
  for (uint64_t id = 1; id < 200000; ++id) {
    const uint16_t short_id = NodeTable::compute_short_id(id);
    if (seen[short_id] == 0) {
      seen[short_id] = id;
    } else {
      id_a = seen[short_id];
      id_b = id;
      break;
    }
  }
  TEST_ASSERT_NOT_EQUAL_UINT64(0, id_a);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, id_b);

  TEST_ASSERT_TRUE(table.upsert_remote(id_a, false, 0, 0, 0, -50, 1, 100));
  TEST_ASSERT_TRUE(table.upsert_remote(id_b, false, 0, 0, 0, -50, 2, 200));

  const uint16_t snapshot_id = table.create_snapshot(300);
  uint8_t buffer[NodeTable::kRecordBytes * 4] = {};
  const size_t bytes = table.get_snapshot_page(snapshot_id, 0, 10, buffer, sizeof(buffer));
  RecordView record{};
  TEST_ASSERT_TRUE(find_record(buffer, bytes, id_a, &record));
  TEST_ASSERT_TRUE((record.flags & 0x08) != 0);
  TEST_ASSERT_TRUE(find_record(buffer, bytes, id_b, &record));
  TEST_ASSERT_TRUE((record.flags & 0x08) != 0);
}

void test_snapshot_consistency() {
  NodeTable table;
  table.set_expected_interval_s(10);
  table.init_self(0xABCDEF0000000001ULL, 0);
  const uint64_t remote_a = 0xABCDEF0000000002ULL;
  const uint64_t remote_b = 0xABCDEF0000000003ULL;
  TEST_ASSERT_TRUE(table.upsert_remote(remote_a, true, 10, 20, 5, -40, 9, 1000));

  const uint16_t snapshot_id = table.create_snapshot(2000);

  TEST_ASSERT_TRUE(table.upsert_remote(remote_b, true, 30, 40, 6, -41, 10, 3000));

  uint8_t buffer[NodeTable::kRecordBytes * 4] = {};
  const size_t bytes = table.get_snapshot_page(snapshot_id, 0, 10, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kRecordBytes * 2, bytes);

  RecordView record{};
  TEST_ASSERT_TRUE(find_record(buffer, bytes, remote_a, &record));
  TEST_ASSERT_EQUAL_UINT16(1, record.last_seen_age_s);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_self_init_and_serialization);
  RUN_TEST(test_remote_upsert_and_fields);
  RUN_TEST(test_grey_transition);
  RUN_TEST(test_eviction_oldest_grey);
  RUN_TEST(test_collision_flagging);
  RUN_TEST(test_snapshot_consistency);
  return UNITY_END();
}
