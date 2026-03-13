#include "services/oled_status.h"

#include <cstdio>
#include <cstring>

namespace naviga {

namespace {

constexpr uint8_t kOledAddress = 0x3C;
constexpr uint32_t kRenderIntervalMs = 500U;

const char* safe_text(const char* text) {
  return text ? text : "-";
}

/** Role as single letter for compact line 1: H=Person, D=Dog, I=Infra. */
char role_to_letter(const char* role_name) {
  if (!role_name) return '?';
  if (role_name[0] == 'D') return 'D';  // Dog
  if (role_name[0] == 'I') return 'I';  // Infra
  return 'H';  // Person / Human
}

/** Format uptime as HH:MM:SS or MM:SS for #450 line 1. */
void format_uptime(uint32_t uptime_ms, char* out, size_t out_len) {
  if (!out || out_len == 0) return;
  const uint32_t total_seconds = uptime_ms / 1000U;
  const uint32_t hours = total_seconds / 3600U;
  const uint32_t minutes = (total_seconds % 3600U) / 60U;
  const uint32_t seconds = total_seconds % 60U;
  if (hours > 0U) {
    std::snprintf(out, out_len, "%02lu:%02lu:%02lu",
                  static_cast<unsigned long>(hours),
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(seconds));
  } else {
    std::snprintf(out, out_len, "%02lu:%02lu",
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(seconds));
  }
}

} // namespace

void OledStatus::init(const HwProfile& profile) {

  Wire.begin(profile.pins.i2c_sda, profile.pins.i2c_scl);
  ready_ = display_.begin(SSD1306_SWITCHCAPVCC, kOledAddress);
  if (!ready_) {
    return;
  }

  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.println("OLED ready");
  display_.display();
}

void OledStatus::update(uint32_t now_ms, const OledStatusData& data) {
  if (!ready_) {
    return;
  }
  if ((now_ms - last_render_ms_) < kRenderIntervalMs) {
    return;
  }
  last_render_ms_ = now_ms;
  render(data);
}

void OledStatus::render(const OledStatusData& data) {
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);

  char uptime_buf[16] = {0};
  format_uptime(data.uptime_ms, uptime_buf, sizeof(uptime_buf));

  // Line 1: display_name (large) <RoleLetter> | uptime
  display_.setTextSize(2);
  display_.setCursor(0, 0);
  display_.print(safe_text(data.display_name));
  display_.setTextSize(1);
  display_.print(" ");
  display_.print(role_to_letter(data.role_name));
  display_.print(" | ");
  display_.print(uptime_buf);

  // Line 2: TX:<power> | N:<n> | Fix:<state>
  display_.setCursor(0, 16);
  display_.print("TX:");
  display_.print(static_cast<unsigned>(data.tx_power_dbm));
  display_.print(" | N:");
  display_.print(static_cast<unsigned long>(data.active_nodes));
  display_.print(" | Fix:");
  display_.print(safe_text(data.fix_state));

  // Line 3: MI:<s> | MD:<m> | MS:<s>
  display_.setCursor(0, 26);
  display_.print("MI:");
  display_.print(static_cast<unsigned>(data.min_interval_sec));
  display_.print(" | MD:");
  display_.print(static_cast<unsigned>(data.min_distance_m));
  display_.print(" | MS:");
  display_.print(static_cast<unsigned>(data.max_silence_10s));

  // Line 4: PosTx:<n> | StTx:<n>
  display_.setCursor(0, 36);
  display_.print("PosTx:");
  display_.print(static_cast<unsigned long>(data.pos_tx));
  display_.print(" | StTx:");
  display_.print(static_cast<unsigned long>(data.st_tx));

  // Line 5: PosRx:<n> | StRx:<n>
  display_.setCursor(0, 46);
  display_.print("PosRx:");
  display_.print(static_cast<unsigned long>(data.pos_rx));
  display_.print(" | StRx:");
  display_.print(static_cast<unsigned long>(data.st_rx));

  display_.display();
}

} // namespace naviga
