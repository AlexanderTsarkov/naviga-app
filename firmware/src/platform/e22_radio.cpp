// Guard: this translation unit must only be compiled for E22 environments.
// PlatformIO compiles all src/ files for every env; the #error in e22_radio.h
// would fire for E220 envs. We guard here instead of in the header so the
// header's #error still catches accidental direct includes.
#if defined(HW_PROFILE_DEVKIT_E22_OLED_GNSS)

#include "platform/e22_radio.h"

#include <algorithm>
#include <cstring>

// E22 library requires frequency band and power variant to be defined at compile time.
// These must match the build_flags in platformio.ini for the E22 env.
// -DFREQUENCY_433 → OPERATING_FREQUENCY = 410 MHz base
// -DE22_30        → POWER_30/27/24/21 dBm variants

namespace naviga {

namespace {

constexpr UART_BPS_RATE kE22BaudRate = UART_BPS_RATE_9600;

// Fixed critical params (not preset-dependent).
constexpr byte kExpectedUartBaudRate     = UART_BPS_9600;
constexpr byte kExpectedUartParity       = MODE_00_8N1;
constexpr byte kExpectedAddressHigh      = 0x00;
constexpr byte kExpectedAddressLow       = 0x00;
constexpr byte kExpectedNetId            = 0x00;
constexpr byte kExpectedFixedTx          = FT_TRANSPARENT_TRANSMISSION;
constexpr byte kExpectedRssiEnable       = RSSI_ENABLED;
constexpr byte kExpectedLbtEnable        = LBT_DISABLED;
constexpr byte kExpectedRepeaterEnable   = REPEATER_DISABLED;
constexpr byte kExpectedSubPacketSetting = SPS_240_00;
constexpr byte kExpectedRssiAmbientNoise = RSSI_AMBIENT_NOISE_ENABLED;

// Preset-derived params (passed in from RadioPreset after normalization).
struct ApplyTarget {
  byte air_data_rate;  // normalized (>= kMinAirRate)
  byte channel;
};

bool critical_params_match(const Configuration& cfg, const ApplyTarget& t) {
  return cfg.SPED.uartBaudRate == kExpectedUartBaudRate &&
         cfg.SPED.uartParity == kExpectedUartParity &&
         cfg.SPED.airDataRate == t.air_data_rate &&
         cfg.ADDH == kExpectedAddressHigh &&
         cfg.ADDL == kExpectedAddressLow &&
         cfg.NETID == kExpectedNetId &&
         cfg.CHAN == t.channel &&
         cfg.TRANSMISSION_MODE.fixedTransmission == kExpectedFixedTx &&
         cfg.TRANSMISSION_MODE.enableRSSI == kExpectedRssiEnable &&
         cfg.TRANSMISSION_MODE.enableLBT == kExpectedLbtEnable &&
         cfg.TRANSMISSION_MODE.enableRepeater == kExpectedRepeaterEnable &&
         cfg.OPTION.subPacketSetting == kExpectedSubPacketSetting &&
         cfg.OPTION.RSSIAmbientNoise == kExpectedRssiAmbientNoise;
}

void apply_critical(Configuration* cfg, const ApplyTarget& t) {
  cfg->SPED.uartBaudRate = kExpectedUartBaudRate;
  cfg->SPED.uartParity = kExpectedUartParity;
  cfg->SPED.airDataRate = t.air_data_rate;
  cfg->ADDH = kExpectedAddressHigh;
  cfg->ADDL = kExpectedAddressLow;
  cfg->NETID = kExpectedNetId;
  cfg->CHAN = t.channel;
  cfg->TRANSMISSION_MODE.fixedTransmission = kExpectedFixedTx;
  cfg->TRANSMISSION_MODE.enableRSSI = kExpectedRssiEnable;
  cfg->TRANSMISSION_MODE.enableLBT = kExpectedLbtEnable;
  cfg->TRANSMISSION_MODE.enableRepeater = kExpectedRepeaterEnable;
  cfg->OPTION.subPacketSetting = kExpectedSubPacketSetting;
  cfg->OPTION.RSSIAmbientNoise = kExpectedRssiAmbientNoise;
}

void describe_mismatch(const Configuration& current, const ApplyTarget& t,
                       char* out, size_t out_len) {
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
  if (current.SPED.uartBaudRate != kExpectedUartBaudRate)              append("baud");
  if (current.SPED.uartParity != kExpectedUartParity)                  append("parity");
  if (current.SPED.airDataRate != t.air_data_rate)                     append("air_rate");
  if (current.ADDH != kExpectedAddressHigh)                            append("addh");
  if (current.ADDL != kExpectedAddressLow)                             append("addl");
  if (current.NETID != kExpectedNetId)                                 append("netid");
  if (current.CHAN != t.channel)                                       append("chan");
  if (current.TRANSMISSION_MODE.fixedTransmission != kExpectedFixedTx) append("fixed_tx");
  if (current.TRANSMISSION_MODE.enableRSSI != kExpectedRssiEnable)     append("rssi");
  if (current.TRANSMISSION_MODE.enableLBT != kExpectedLbtEnable)       append("lbt");
  if (current.TRANSMISSION_MODE.enableRepeater != kExpectedRepeaterEnable) append("repeater");
  if (current.OPTION.subPacketSetting != kExpectedSubPacketSetting)    append("subpkt");
  if (current.OPTION.RSSIAmbientNoise != kExpectedRssiAmbientNoise)    append("rssi_amb");
}

} // namespace

E22Radio::E22Radio(const Pins& pins)
    : pins_(pins),
      radio_(static_cast<byte>(pins.lora_tx), static_cast<byte>(pins.lora_rx), &serial_,
             static_cast<byte>(pins.lora_aux), static_cast<byte>(pins.lora_m0),
             static_cast<byte>(pins.lora_m1), kE22BaudRate, SERIAL_8N1) {}

bool E22Radio::begin() {
  return begin(get_radio_preset(RadioPresetId::Default));
}

bool E22Radio::begin(const RadioPreset& preset) {
  last_boot_result_ = E22BootConfigResult::Ok;
  last_boot_message_[0] = '\0';

  // Normalize airRate: E22-400T30D V2 clamps < 2 to 2 (issue #336).
  uint8_t norm_air_rate = preset.air_rate;
  const bool clamped = normalize_air_rate(preset.air_rate, &norm_air_rate);
  if (clamped) {
    // Log is emitted via boot_config_message() after begin() returns.
    // We store it now so the caller sees it regardless of repair outcome.
    std::snprintf(last_boot_message_, kBootMessageLen,
                  "air_rate clamped: req=%u norm=%u", preset.air_rate, norm_air_rate);
    last_boot_result_ = E22BootConfigResult::Repaired;
  }

  const ApplyTarget target{/*.air_data_rate=*/static_cast<byte>(norm_air_rate),
                            /*.channel=*/static_cast<byte>(preset.channel)};

  ready_ = radio_.begin();
  if (!ready_) {
    return false;
  }

  // E22 config mode = M0=LOW, M1=HIGH. Start from Normal (M0=LOW, M1=LOW)
  // and let the library switch to config mode. Give the module settling time.
  digitalWrite(static_cast<uint8_t>(pins_.lora_m0), LOW);
  digitalWrite(static_cast<uint8_t>(pins_.lora_m1), LOW);
  delay(200);
  serial_.flush();
  while (serial_.available()) serial_.read();

  // Verify-and-repair critical params every boot.
  ResponseStructContainer config = radio_.getConfiguration();
  if (config.status.code != E22_SUCCESS || !config.data) {
    config.close();
    last_boot_result_ = E22BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "read failed");
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;
  }

  Configuration* cfg = reinterpret_cast<Configuration*>(config.data);
  if (critical_params_match(*cfg, target)) {
    rssi_enabled_ = (cfg->TRANSMISSION_MODE.enableRSSI == RSSI_ENABLED);
    config.close();
    // If we only had a clamp (no structural mismatch), keep Repaired status.
    return true;
  }

  // Mismatch: repair once (apply target, write, re-read, verify).
  // Preserve clamp note if already set, otherwise record mismatch fields.
  if (!clamped) {
    describe_mismatch(*cfg, target, last_boot_message_, kBootMessageLen);
  }
  apply_critical(cfg, target);
  ResponseStatus write_status = radio_.setConfiguration(*cfg, WRITE_CFG_PWR_DWN_SAVE);
  config.close();

  if (write_status.code != E22_SUCCESS) {
    last_boot_result_ = E22BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "write failed");
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;
  }

  ResponseStructContainer config2 = radio_.getConfiguration();
  if (config2.status.code != E22_SUCCESS || !config2.data) {
    last_boot_result_ = E22BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen, "re-read failed");
    config2.close();
    rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED);
    return true;
  }

  Configuration* cfg2 = reinterpret_cast<Configuration*>(config2.data);
  const RadioPresetVerifyResult vr = verify_preset_readback(
      RadioPreset{norm_air_rate, preset.channel, preset.tx_power_dbm},
      cfg2->SPED.airDataRate,
      cfg2->CHAN);
  rssi_enabled_ = (cfg2->TRANSMISSION_MODE.enableRSSI == RSSI_ENABLED);

  if (vr.all_ok()) {
    last_boot_result_ = E22BootConfigResult::Repaired;
    // last_boot_message_ already has mismatch list or clamp note.
  } else {
    last_boot_result_ = E22BootConfigResult::RepairFailed;
    std::snprintf(last_boot_message_, kBootMessageLen,
                  "verify failed: req=%u/%u norm=%u rb=%u/%u",
                  preset.air_rate, preset.channel,
                  norm_air_rate,
                  cfg2->SPED.airDataRate, cfg2->CHAN);
  }
  config2.close();

  return true;
}

bool E22Radio::is_ready() const {
  return ready_;
}

bool E22Radio::rssi_available() const {
  return rssi_enabled_;
}

bool E22Radio::send(const uint8_t* data, size_t len) {
  if (!ready_ || !data || len == 0 || len > MAX_SIZE_TX_PACKET) {
    return false;
  }
  ResponseStatus status = radio_.sendMessage(data, static_cast<uint8_t>(len));
  return status.code == E22_SUCCESS;
}

bool E22Radio::recv(uint8_t* out, size_t max_len, size_t* out_len) {
  if (!ready_ || !out_len || max_len == 0) {
    return false;
  }
  if (radio_.available() <= 0) {
    *out_len = 0;
    return false;
  }

  // Use String-based receiveMessage() — reads until UART buffer is drained.
  // receiveMessage(size) fails with DATA_SIZE_NOT_MATCH for variable-length frames.
  ResponseContainer response = rssi_enabled_
                                   ? radio_.receiveMessageRSSI()
                                   : radio_.receiveMessage();
  if (response.status.code != E22_SUCCESS) {
    *out_len = 0;
    return false;
  }

  const String& data = response.data;
  const size_t n = static_cast<size_t>(data.length());
  if (n == 0 || n > max_len) {
    *out_len = 0;
    return false;
  }

  std::memcpy(out, data.c_str(), n);
  *out_len = n;

  if (rssi_enabled_) {
    last_rssi_dbm_ = static_cast<int8_t>(response.rssi);
  }
  return true;
}

int8_t E22Radio::last_rssi_dbm() const {
  return last_rssi_dbm_;
}

RadioBootConfigResult E22Radio::boot_config_result() const {
  switch (last_boot_result_) {
    case E22BootConfigResult::Repaired:    return RadioBootConfigResult::Repaired;
    case E22BootConfigResult::RepairFailed: return RadioBootConfigResult::RepairFailed;
    default:                               return RadioBootConfigResult::Ok;
  }
}

const char* E22Radio::boot_config_message() const {
  return last_boot_message_;
}

} // namespace naviga

#endif // HW_PROFILE_DEVKIT_E22_OLED_GNSS
