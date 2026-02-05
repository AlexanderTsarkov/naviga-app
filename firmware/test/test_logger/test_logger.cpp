#include <unity.h>

#include <cstdint>

#include "../../src/domain/logger.h"
#include "../../src/domain/logger.cpp"

using naviga::domain::LogEventId;
using naviga::domain::LogLevel;
using naviga::domain::LogRecordView;
using naviga::domain::Logger;

struct EventCapture {
  LogEventId ids[8] = {};
  size_t count = 0;
};

void capture_event(void* ctx, const LogRecordView& record) {
  auto* capture = static_cast<EventCapture*>(ctx);
  if (capture->count < 8) {
    capture->ids[capture->count++] = record.event_id;
  }
}

void test_wraparound_drop_oldest() {
  uint8_t storage[32] = {};
  Logger logger(storage, sizeof(storage));

  uint8_t payload[4] = {1, 2, 3, 4};
  TEST_ASSERT_TRUE(logger.log(1, LogEventId::RADIO_TX_OK, LogLevel::kInfo, payload, 4));
  TEST_ASSERT_TRUE(logger.log(2, LogEventId::RADIO_TX_ERR, LogLevel::kInfo, payload, 4));
  TEST_ASSERT_TRUE(logger.log(3, LogEventId::RADIO_RX_OK, LogLevel::kInfo, payload, 4));

  EventCapture capture{};
  logger.for_each_record(capture_event, &capture);
  TEST_ASSERT_EQUAL_UINT32(2, capture.count);
  TEST_ASSERT_EQUAL(LogEventId::RADIO_TX_ERR, capture.ids[0]);
  TEST_ASSERT_EQUAL(LogEventId::RADIO_RX_OK, capture.ids[1]);
}

void test_le_serialization() {
  uint8_t storage[64] = {};
  Logger logger(storage, sizeof(storage));

  const uint8_t payload[3] = {0xAA, 0xBB, 0xCC};
  TEST_ASSERT_TRUE(logger.log(0x11223344, static_cast<LogEventId>(0x5566),
                              LogLevel::kWarn, payload, sizeof(payload)));

  uint8_t raw[16] = {};
  const size_t bytes = logger.copy_raw(raw, sizeof(raw));
  TEST_ASSERT_EQUAL_UINT32(11, bytes);

  const uint8_t expected[11] = {
      0x44, 0x33, 0x22, 0x11, // t_ms LE
      0x66, 0x55,             // event_id LE
      0x02,                   // LogLevel::kWarn
      0x03,                   // payload length
      0xAA, 0xBB, 0xCC         // payload
  };
  for (size_t i = 0; i < sizeof(expected); ++i) {
    TEST_ASSERT_EQUAL_UINT8(expected[i], raw[i]);
  }
}

void test_drain_order_and_clear() {
  uint8_t storage[64] = {};
  Logger logger(storage, sizeof(storage));

  TEST_ASSERT_TRUE(logger.log(10, LogEventId::BLE_CONNECT, LogLevel::kInfo));
  TEST_ASSERT_TRUE(logger.log(20, LogEventId::BLE_DISCONNECT, LogLevel::kInfo));

  EventCapture capture{};
  logger.drain(capture_event, &capture);

  TEST_ASSERT_EQUAL_UINT32(2, capture.count);
  TEST_ASSERT_EQUAL(LogEventId::BLE_CONNECT, capture.ids[0]);
  TEST_ASSERT_EQUAL(LogEventId::BLE_DISCONNECT, capture.ids[1]);
  TEST_ASSERT_EQUAL_UINT32(0, logger.size());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_wraparound_drop_oldest);
  RUN_TEST(test_le_serialization);
  RUN_TEST(test_drain_order_and_clear);
  return UNITY_END();
}
