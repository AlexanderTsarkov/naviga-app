#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

uint64_t get_device_full_id_u64();
uint16_t get_device_short_id_u16();
void get_device_mac_bytes(uint8_t out_mac[6]);

uint64_t full_id_from_mac(const uint8_t mac[6]);
uint16_t short_id_from_mac(const uint8_t mac[6]);

void format_full_id_u64_hex(uint64_t full_id, char* out, size_t out_len);
void format_full_id_mac_hex(const uint8_t mac[6], char* out, size_t out_len);
void format_short_id_hex(uint16_t short_id, char* out, size_t out_len);
void format_mac_colon_hex(const uint8_t mac[6], char* out, size_t out_len);

} // namespace naviga
