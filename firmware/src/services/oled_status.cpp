#include "services/oled_status.h"

namespace naviga {

namespace {

constexpr uint8_t kOledAddress = 0x3C;
constexpr uint32_t kRenderIntervalMs = 250U;

const char* role_label(RadioRole role) {
  return role == RadioRole::INIT ? "INIT (PING)" : "RESP (PONG)";
}

} // namespace

void OledStatus::init(const HwProfile& profile, const char* firmware_version, RadioRole role) {
  role_ = role;
  profile_name_ = profile.name;
  firmware_version_ = firmware_version;

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

void OledStatus::update(uint32_t now_ms, const RadioSmokeStats& stats) {
  if (!ready_) {
    return;
  }
  if ((now_ms - last_render_ms_) < kRenderIntervalMs) {
    return;
  }
  last_render_ms_ = now_ms;
  render(stats);
}

void OledStatus::render(const RadioSmokeStats& stats) {
  display_.clearDisplay();

  display_.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  display_.setTextColor(SSD1306_BLACK);
  display_.setTextSize(1);
  display_.setCursor(0, 4);
  display_.print(role_label(role_));

  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 18);
  display_.print("env: ");
  display_.print(profile_name_ ? profile_name_ : "-");

  display_.setCursor(0, 26);
  display_.print("fw: ");
  display_.print(firmware_version_ ? firmware_version_ : "-");

  display_.setCursor(0, 34);
  display_.print("radio: ");
  display_.print(stats.radio_ready ? "OK" : "ERR");

  display_.setCursor(0, 42);
  display_.print("tx ");
  display_.print(stats.tx_count);
  display_.print(" rx ");
  display_.print(stats.rx_count);

  display_.setCursor(0, 50);
  display_.print("seq ");
  display_.print(stats.last_seq);

  display_.setCursor(0, 56);
  display_.print("tx@");
  display_.print(stats.last_tx_ms);
  display_.print(" rx@");
  display_.print(stats.last_rx_ms);

  if (stats.rssi_available) {
    display_.setCursor(96, 34);
    display_.print("R");
    display_.print(stats.last_rssi_dbm);
  }

  display_.display();
}

} // namespace naviga
