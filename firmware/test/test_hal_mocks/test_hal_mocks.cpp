#include <unity.h>

#include <cstdint>

#include "naviga/hal/interfaces.h"
#include "naviga/hal/mocks/mock_ble_transport.h"
#include "naviga/hal/mocks/mock_gnss.h"
#include "naviga/hal/mocks/mock_log.h"
#include "naviga/hal/mocks/mock_radio.h"

using namespace naviga;

void test_mock_radio_send_and_recv() {
  MockRadio radio;
  uint8_t payload[3] = {1, 2, 3};
  TEST_ASSERT_TRUE(radio.send(payload, sizeof(payload)));
  TEST_ASSERT_EQUAL_UINT32(1, radio.tx_count());
  TEST_ASSERT_EQUAL_UINT32(3, radio.last_tx_len());

  uint8_t rx_payload[2] = {9, 8};
  radio.inject_rx(rx_payload, sizeof(rx_payload), -42);
  uint8_t out[8] = {0};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(radio.recv(out, sizeof(out), &out_len));
  TEST_ASSERT_EQUAL_UINT32(2, out_len);
  TEST_ASSERT_EQUAL_UINT8(9, out[0]);
  TEST_ASSERT_EQUAL_UINT8(8, out[1]);
  TEST_ASSERT_EQUAL_INT8(-42, radio.last_rssi_dbm());
}

void test_mock_ble_transport_store() {
  MockBleTransport ble;
  uint8_t dev_info[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  ble.set_device_info(dev_info, sizeof(dev_info));
  TEST_ASSERT_EQUAL_UINT32(4, ble.device_info_len());
  TEST_ASSERT_EQUAL_UINT8(0xAA, ble.device_info()[0]);

  uint8_t page0[3] = {1, 2, 3};
  ble.set_node_table_page(0, page0, sizeof(page0));
  TEST_ASSERT_EQUAL_UINT32(3, ble.page_len(0));
}

void test_mock_gnss_snapshot() {
  MockGnss gnss;
  GnssSnapshot snap{true, 123, 456};
  gnss.set_snapshot(snap);
  GnssSnapshot out{};
  TEST_ASSERT_TRUE(gnss.get_snapshot(&out));
  TEST_ASSERT_TRUE(out.has_fix);
  TEST_ASSERT_EQUAL_INT32(123, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(456, out.lon_e7);
}

void test_mock_log() {
  MockLog log;
  uint8_t data[2] = {0, 1};
  log.log("TEST", data, sizeof(data));
  TEST_ASSERT_EQUAL_STRING("TEST", log.last_tag());
  TEST_ASSERT_EQUAL_UINT32(2, log.last_len());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mock_radio_send_and_recv);
  RUN_TEST(test_mock_ble_transport_store);
  RUN_TEST(test_mock_gnss_snapshot);
  RUN_TEST(test_mock_log);
  return UNITY_END();
}
