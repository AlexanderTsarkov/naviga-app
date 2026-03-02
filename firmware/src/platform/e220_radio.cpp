#include "platform/e220_radio.h"

#include <algorithm>
#include <cstring>

namespace naviga {

namespace {

constexpr UART_BPS_RATE kE220BaudRate = UART_BPS_RATE_9600;

// Expected critical params per module_boot_config_v0 (verify-and-repair every boot).
constexpr byte kExpectedUartBaudRate = UART_BPS_9600;
constexpr byte kExpectedUartParity = MODE_00_8N1;
constexpr byte kExpectedRssiEnable = RSSI_ENABLED;
constexpr byte kExpectedLbtEnable = 0;  // OFF — policy: sense unsupported, jitter-only.
constexpr byte kExpectedSubPacketSetting = SPS_200_00;
constexpr byte kExpectedRssiAmbientNoise = RSSI_AMBIENT_NOISE_ENABLED;  // Align with RSSI byte ON.

// Returns true if critical fields of cfg match expected values.
bool critical_params_match(const Configuration& cfg) {
  return cfg.SPED.uartBaudRate == kExpectedUartBaudRate &&
         cfg.SPED.uartParity == kExpectedUartParity &&
         cfg.TRANSMISSION_MODE.enableRSSI == kExpectedRssiEnable &&
         cfg.TRANSMISSION_MODE.enableLBT == kExpectedLbtEnable &&
         cfg.OPTION.subPacketSetting == kExpectedSubPacketSetting &&
         cfg.OPTION.RSSIAmbientNoise == kExpectedRssiAmbientNoise;
}

// Overwrites only critical fields in cfg with expected values (rest unchanged).
void apply_expected_critical(Configuration* cfg) {
  cfg->SPED.uartBaudRate = kExpectedUartBaudRate;
  cfg->SPED.uartParity = kExpectedUartParity;
  cfg->TRANSMISSION_MODE.enableRSSI = kExpectedRssiEnable;
  cfg->TRANSMISSION_MODE.enableLBT = kExpectedLbtEnable;
  cfg->OPTION.subPacketSetting = kExpectedSubPacketSetting;
  cfg->OPTION.RSSIAmbientNoise = kExpectedRssiAmbientNoise;
}

// Builds a short comma-separated list of what differed (for log).
void describe_mismatch(const Configuration& current, char* out, size_t out_len) {
  if (out_len == 0) return;
  char* p = out;
  size_t remain = out_len;
  auto append = [&](const char* tag) {
    const size_t tag_len = std::strlen(tag);
    if (p > out) {
      if (remain >= 3) { *p++ = ','; *p++ = ' '; remain -= 2; }
    }
    if (remain > tag_len) {
      std::memcpy(p, tag, tag_len + 1);
      p += tag_len;
      remain -= tag_len;
    }
  };
  if (current.SPED.uartBaudRate != kExpectedUartBaudRate) append("baud");
  if (current.SPED.uartParity != kExpectedUartParity) append("parity");
  if (current.TRANSMISSION_MODE.enableRSSI != kExpectedRssiEnable) append("rssi");
  if (current.TRANSMISSION_MODE.enableLBT != kExpectedLbtEnable) append("lbt");
  if (current.OPTION.subPacketSetting != kExpectedSubPacketSetting) append("subpkt");
  if (current.OPTION.RSSIAmbientNoise != kExpectedRssiAmbientNoise) append("rssi_amb");
}

} // namespace

E220Radio::E220Radio(const Pins& pins)
    : pins_(pins),
      radio_(static_cast<byte>(pins.lora_tx), static_cast<byte>(pins.lora_rx), &serial_,
             static_cast<byte>(pins.lora_aux), static_cast<byte>(pins.lora_m0),
             static_cast<byte>(pins.lora_m1), kE220BaudRate, SERIAL_8N1) {}

bool E220Radio::begin() {
  last_boot_result_ = E220BootConfigResult::Ok;
  last_boot_message_[0] = '\0';

  ready_ = radio_.begin();
  if (!ready_) {
    return false;
  }

  // #326 fix: pre-assert config mode (M0=M1=HIGH) and hold 200 ms before getConfiguration().
  // Root cause: the library's setMode(MODE_3_PROGRAM) checks AUX immediately after asserting
  // M0/M1 HIGH; since AUX is already HIGH at idle the library does not wait for the actual
  // mode-switch settling time, causing HEAD_NOT_RECOGNIZED / NO_RESPONSE on getConfiguration.
  // Pre-asserting here gives the module the required settling time before the library call.
  digitalWrite(static_cast<uint8_t>(pins_.lora_m0), HIGH);
  digitalWrite(static_cast<uint8_t>(pins_.lora_m1), HIGH);
  delay(200);
  serial_.flush();
  while (serial_.available()) serial_.read();

  // Verify-and-repair critical params every boot (module_boot_config_v0).
  ResponseStructContainer config = radio_.getConfiguration();
  if (config.status.code != E220_SUCCESS || !config.data) {
    config.close();
    last_boot_result_ = E220BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "read failed");
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;  // Proceed; caller can log warning.
  }

  Configuration* cfg = reinterpret_cast<Configuration*>(config.data);
  if (critical_params_match(*cfg)) {
    rssi_enabled_ = (cfg->TRANSMISSION_MODE.enableRSSI == RSSI_ENABLED);
    config.close();
    return true;
  }

  // Mismatch: repair once (apply expected critical, write, re-read, verify).
  describe_mismatch(*cfg, last_boot_message_, kBootMessageLen);
  apply_expected_critical(cfg);
  ResponseStatus write_status = radio_.setConfiguration(*cfg, WRITE_CFG_PWR_DWN_LOSE);
  config.close();

  if (write_status.code != E220_SUCCESS) {
    last_boot_result_ = E220BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "write failed");
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;  // Module is up; use expected params for our state.
  }

  ResponseStructContainer config2 = radio_.getConfiguration();
  if (config2.status.code != E220_SUCCESS || !config2.data) {
    last_boot_result_ = E220BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "re-read failed");
    config2.close();
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;
  }

  Configuration* cfg2 = reinterpret_cast<Configuration*>(config2.data);
  bool verified = critical_params_match(*cfg2);
  rssi_enabled_ = (cfg2->TRANSMISSION_MODE.enableRSSI == RSSI_ENABLED);
  config2.close();

  if (verified) {
    last_boot_result_ = E220BootConfigResult::Repaired;
    // last_boot_message_ already set to mismatch list (what we repaired).
  } else {
    last_boot_result_ = E220BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "verify failed");
  }

  return true;
}

bool E220Radio::is_ready() const {
  return ready_;
}

bool E220Radio::rssi_available() const {
  return rssi_enabled_;
}

bool E220Radio::send(const uint8_t* data, size_t len) {
  if (!ready_ || !data || len == 0 || len > MAX_SIZE_TX_PACKET) {
    return false;
  }
  ResponseStatus status = radio_.sendMessage(data, static_cast<uint8_t>(len));
  return status.code == E220_SUCCESS;
}

bool E220Radio::recv(uint8_t* out, size_t max_len, size_t* out_len) {
  if (!ready_ || !out_len || max_len == 0) {
    return false;
  }
  if (radio_.available() <= 0) {
    *out_len = 0;
    return false;
  }

  ResponseStructContainer response = rssi_enabled_
                                         ? radio_.receiveMessageRSSI(static_cast<uint8_t>(max_len))
                                         : radio_.receiveMessage(static_cast<uint8_t>(max_len));
  if (response.status.code != E220_SUCCESS || !response.data) {
    if (response.data) {
      response.close();
    }
    *out_len = 0;
    return false;
  }

  const size_t copy_len = max_len;
  std::memcpy(out, response.data, copy_len);
  *out_len = copy_len;
  if (rssi_enabled_) {
    last_rssi_dbm_ = static_cast<int8_t>(response.rssi);
  }
  response.close();
  return true;
}

int8_t E220Radio::last_rssi_dbm() const {
  return last_rssi_dbm_;
}

} // namespace naviga
