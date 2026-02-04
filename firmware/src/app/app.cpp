#include "app/app.h"

#include "app/app_services.h"

namespace naviga {

namespace {

AppServices services;

} // namespace

void app_init() {
  services.init();
}

void app_tick(uint32_t now_ms) {
  services.tick(now_ms);
}

} // namespace naviga
