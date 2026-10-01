#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>

namespace esphome::tailscale {
// Tailscale map messages have a little-endian length prefix. Keep partial
// messages between nonblocking polls, and return one message at a time even
// when a DATA frame contains several. Storage is allocated lazily for the
// streaming session.
template <size_t Capacity = 65536> class MapMessages {
public:
  enum class Result { More, Ready, Invalid };
  Result feed(const uint8_t *input, size_t size, size_t &consumed) {
    consumed = 0;
    if (invalid_ || (!input && size))
      return Result::Invalid;
    while (consumed < size) {
      if (prefixSize_ < 4) {
        prefix_[prefixSize_++] = input[consumed++];
        if (prefixSize_ < 4)
          continue;
        wanted_ = uint32_t(prefix_[0]) | (uint32_t(prefix_[1]) << 8) |
                  (uint32_t(prefix_[2]) << 16) | (uint32_t(prefix_[3]) << 24);
        if (!wanted_ || wanted_ >= Capacity) {
          invalid_ = true;
          return Result::Invalid;
        }
      }
      if (!message_)
        message_.reset(new (std::nothrow) char[Capacity]);
      if (!message_) {
        invalid_ = true;
        return Result::Invalid;
      }
      const size_t available = size - consumed;
      const size_t needed = wanted_ - used_;
      const size_t count = available < needed ? available : needed;
      memcpy(message_.get() + used_, input + consumed, count);
      consumed += count;
      used_ += count;
      if (used_ == wanted_) {
        message_[used_] = '\0';
        delivered_ = used_;
        used_ = prefixSize_ = wanted_ = 0;
        return Result::Ready;
      }
    }
    return Result::More;
  }
  void reset() {
    prefixSize_ = used_ = wanted_ = delivered_ = 0;
    invalid_ = false;
  }
  const char *data() const { return message_.get(); }
  size_t size() const { return delivered_; }

private:
  std::unique_ptr<char[]> message_;
  uint8_t prefix_[4]{};
  size_t prefixSize_{0}, used_{0}, wanted_{0}, delivered_{0};
  bool invalid_{false};
};
} // namespace esphome::tailscale
