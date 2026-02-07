#include "services/oled_status.h"

#include <cstdio>

namespace naviga {

namespace {

constexpr uint8_t kOledAddress = 0x3C;
constexpr uint32_t kRenderIntervalMs = 500U;

const char* safe_text(const char* text) {
  return text ? text : "-";
}

void format_uptime_mmss(uint32_t uptime_ms, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  const uint32_t total_seconds = uptime_ms / 1000U;
  const uint32_t minutes = total_seconds / 60U;
  const uint32_t seconds = total_seconds % 60U;
  std::snprintf(out, out_len, "%02lu:%02lu",
                static_cast<unsigned long>(minutes),
                static_cast<unsigned long>(seconds));
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

void OledStatus::update(uint32_t now_ms, const RadioSmokeStats& stats, const OledStatusData& data) {
  if (!ready_) {
    return;
  }
  if ((now_ms - last_render_ms_) < kRenderIntervalMs) {
    return;
  }
  last_render_ms_ = now_ms;
  render(stats, data);
}

void OledStatus::render(const RadioSmokeStats& stats, const OledStatusData& data) {
  display_.clearDisplay();

  display_.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  display_.setTextColor(SSD1306_BLACK);
  display_.setTextSize(2);
  display_.setCursor(0, 0);
  display_.print("ID:");
  display_.print(safe_text(data.short_id));

  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 18);
  display_.print("FW: ");
  display_.print(safe_text(data.firmware_version));

  display_.setCursor(0, 26);
  display_.print("BLE: ");
  display_.print(data.ble_connected ? "CONN" : "ADV");

  char uptime_buf[12] = {0};
  format_uptime_mmss(data.uptime_ms, uptime_buf, sizeof(uptime_buf));
  display_.setCursor(0, 34);
  display_.print("NODES:");
  display_.print(static_cast<unsigned long>(data.nodes_seen));
  display_.print(" UPT:");
  display_.print(uptime_buf);

  display_.setCursor(0, 42);
  display_.print("SEQ:");
  display_.print(data.geo_seq);
  display_.print(" T:");
  display_.print(stats.tx_count);
  display_.print(" R:");
  display_.print(stats.rx_count);

  display_.display();
}

} // namespace naviga
