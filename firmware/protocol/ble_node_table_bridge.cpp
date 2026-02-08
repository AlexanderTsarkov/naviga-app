#include "ble_node_table_bridge.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace naviga {
namespace protocol {

namespace {

constexpr size_t kMaxDeviceInfoLen = 256;
constexpr size_t kPageHeaderBytes = 10;

void write_u16_le(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32_le(uint8_t* out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void write_u64_le(uint8_t* out, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
  }
}

void append_u8(uint8_t* buffer, size_t& offset, size_t capacity, uint8_t value) {
  if (offset < capacity) {
    buffer[offset++] = value;
  }
}

void append_u16_le(uint8_t* buffer, size_t& offset, size_t capacity, uint16_t value) {
  if (offset + 1 < capacity) {
    buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
  }
}

void append_u32_le(uint8_t* buffer, size_t& offset, size_t capacity, uint32_t value) {
  if (offset + 3 < capacity) {
    buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 24) & 0xFF);
  }
}

void append_u64_le(uint8_t* buffer, size_t& offset, size_t capacity, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    if (offset < capacity) {
      buffer[offset++] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
    }
  }
}

void append_str(uint8_t* buffer, size_t& offset, size_t capacity, const char* value) {
  if (!value) {
    append_u8(buffer, offset, capacity, 0);
    return;
  }
  const size_t len = std::min<size_t>(255, std::strlen(value));
  append_u8(buffer, offset, capacity, static_cast<uint8_t>(len));
  const size_t copy_len = std::min(len, capacity - offset);
  if (copy_len > 0) {
    std::memcpy(buffer + offset, value, copy_len);
    offset += copy_len;
  }
}

} // namespace

bool BleNodeTableBridge::update_device_info(const DeviceInfoModel& model,
                                            IBleTransport& transport) const {
  std::array<uint8_t, kMaxDeviceInfoLen> buffer{};
  size_t offset = 0;

  append_u8(buffer.data(), offset, buffer.size(), model.format_ver);
  append_u8(buffer.data(), offset, buffer.size(), model.ble_schema_ver);
  if (model.include_radio_proto_ver) {
    append_u8(buffer.data(), offset, buffer.size(), model.radio_proto_ver);
  }
  append_u64_le(buffer.data(), offset, buffer.size(), model.node_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.short_id);
  append_u8(buffer.data(), offset, buffer.size(), model.device_type);
  append_str(buffer.data(), offset, buffer.size(), model.firmware_version);
  append_str(buffer.data(), offset, buffer.size(), model.radio_module_model);
  append_u16_le(buffer.data(), offset, buffer.size(), model.band_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.power_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.power_max);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_max);
  append_u8(buffer.data(), offset, buffer.size(), model.network_mode);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.public_channel_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.private_channel_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.private_channel_max);
  append_u32_le(buffer.data(), offset, buffer.size(), model.capabilities);

  transport.set_device_info(buffer.data(), offset);
  return offset > 0;
}

bool BleNodeTableBridge::update_node_table(uint32_t now_ms,
                                           domain::NodeTable& table,
                                           IBleTransport& transport) const {
  uint16_t req_snapshot_id = 0;
  uint16_t req_page_index = 0;
  if (!transport.get_node_table_request(&req_snapshot_id, &req_page_index)) {
    req_snapshot_id = 0;
    req_page_index = 0;
  }

  uint16_t snapshot_id = req_snapshot_id;
  size_t total_nodes = table.size();
  size_t page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
  size_t page_index = req_page_index;

  bool snapshot_reset = false;
  if (snapshot_id == 0) {
    snapshot_id = table.create_snapshot(now_ms);
    total_nodes = table.size();
    page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
    page_index = 0;
    snapshot_reset = true;
  }

  std::array<uint8_t, kPageHeaderBytes + domain::NodeTable::kRecordBytes * kPageSize> buffer{};
  size_t record_bytes = 0;
  if (total_nodes > 0 && page_index < page_count) {
    record_bytes = table.get_snapshot_page(snapshot_id,
                                           page_index,
                                           kPageSize,
                                           buffer.data() + kPageHeaderBytes,
                                           buffer.size() - kPageHeaderBytes);
    if (record_bytes == 0 && !snapshot_reset) {
      snapshot_id = table.create_snapshot(now_ms);
      total_nodes = table.size();
      page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
      page_index = 0;
      record_bytes = table.get_snapshot_page(snapshot_id,
                                             page_index,
                                             kPageSize,
                                             buffer.data() + kPageHeaderBytes,
                                             buffer.size() - kPageHeaderBytes);
    }
  }

  write_u16_le(buffer.data(), snapshot_id);
  write_u16_le(buffer.data() + 2, static_cast<uint16_t>(total_nodes));
  write_u16_le(buffer.data() + 4, static_cast<uint16_t>(page_index));
  buffer[6] = kPageSize;
  write_u16_le(buffer.data() + 7, static_cast<uint16_t>(page_count));
  buffer[9] = kRecordFormatVer;

  const size_t payload_len = kPageHeaderBytes + record_bytes;
  transport.set_node_table_response(buffer.data(), payload_len);
  return true;
}

bool BleNodeTableBridge::update_all(uint32_t now_ms,
                                    const DeviceInfoModel& model,
                                    domain::NodeTable& table,
                                    IBleTransport& transport) const {
  const bool ok_info = update_device_info(model, transport);
  const bool ok_table = update_node_table(now_ms, table, transport);
  return ok_info && ok_table;
}

} // namespace protocol
} // namespace naviga
