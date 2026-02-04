#pragma once

#include <cstddef>
#include <cstdint>

#include "hal/interfaces.h"

namespace naviga {

class MockLog : public ILog {
 public:
  void log(const char* tag, const uint8_t* data, size_t len) override;

  const char* last_tag() const;
  size_t last_len() const;

 private:
  char last_tag_[32] = {0};
  size_t last_len_ = 0;
};

} // namespace naviga
