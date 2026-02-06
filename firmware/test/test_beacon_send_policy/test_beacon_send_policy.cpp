#include <unity.h>

#include <cstdint>

#include "../../src/domain/beacon_send_policy.h"
#include "../../src/domain/beacon_send_policy.cpp"
#include "../../lib/NavigaCore/include/naviga/hal/mocks/mock_channel_sense.h"
#include "../../lib/NavigaCore/src/mocks/mock_channel_sense.cpp"

using naviga::ChannelSenseResult;
using naviga::ChannelSenseState;
using naviga::MockChannelSense;
using naviga::domain::BeaconSendPolicy;

void test_sense_unsupported_allows_send() {
  BeaconSendPolicy policy;
  policy.init(1);
  policy.set_jitter_ms(0);
  policy.enable_sense(true);

  MockChannelSense sense;
  sense.set_can_sense(false);

  policy.on_payload_built(1000);
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1000));
  TEST_ASSERT_FALSE(policy.should_sense(&sense));
}

void test_sense_busy_defers() {
  BeaconSendPolicy policy;
  policy.init(1);
  policy.set_jitter_ms(0);
  policy.set_backoff_ms(100, 1000);
  policy.enable_sense(true);

  MockChannelSense sense;
  sense.set_next_result({ChannelSenseState::BUSY, -80});

  policy.on_payload_built(1000);
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1000));
  TEST_ASSERT_TRUE(policy.should_sense(&sense));

  policy.on_channel_busy(1000);
  TEST_ASSERT_FALSE(policy.ready_to_attempt(1000));
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1100));
}

void test_send_fail_increases_backoff() {
  BeaconSendPolicy policy;
  policy.init(1);
  policy.set_jitter_ms(0);
  policy.set_backoff_ms(200, 1000);

  policy.on_payload_built(1000);
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1000));

  policy.on_send_result(false, 1000);
  TEST_ASSERT_FALSE(policy.ready_to_attempt(1000));
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1200));
}

void test_send_success_resets_backoff() {
  BeaconSendPolicy policy;
  policy.init(1);
  policy.set_jitter_ms(0);
  policy.set_backoff_ms(200, 1000);

  policy.on_payload_built(1000);
  policy.on_send_result(false, 1000);
  TEST_ASSERT_TRUE(policy.ready_to_attempt(1200));

  policy.on_send_result(true, 1200);
  TEST_ASSERT_FALSE(policy.has_pending());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sense_unsupported_allows_send);
  RUN_TEST(test_sense_busy_defers);
  RUN_TEST(test_send_fail_increases_backoff);
  RUN_TEST(test_send_success_resets_backoff);
  return UNITY_END();
}
