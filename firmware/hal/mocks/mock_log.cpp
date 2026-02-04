#include "hal/mocks/mock_log.h"

#include <cstring>

namespace naviga {

void MockLog::log(const char* tag, const uint8_t* /*data*/, size_t len) {
  if (tag) {
    std::strncpy(last_tag_, tag, sizeof(last_tag_) - 1);
    last_tag_[sizeof(last_tag_) - 1] = '\0';
  } else {
    last_tag_[0] = '\0';
  }
  last_len_ = len;
}

const char* MockLog::last_tag() const {
  return last_tag_;
}

size_t MockLog::last_len() const {
  return last_len_;
}

} // namespace naviga
