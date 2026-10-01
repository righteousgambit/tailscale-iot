#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace esphome::tailscale::control_records {
// Official controlbase records are at most 4096 bytes; the WebSocket layer
// delivers at most 8192. One incomplete record plus one chunk fits here.
class Records {
public:
  enum class State { Incomplete, Invalid, Ready };
  static constexpr size_t Capacity = 12288;
  bool append(const uint8_t *bytes, size_t size) {
    if ((!bytes && size) || size > Capacity - used_)
      return false;
    if (size)
      memcpy(buffer_ + used_, bytes, size);
    used_ += size;
    return true;
  }
  State state() const {
    if (used_ < 3)
      return State::Incomplete;
    const size_t size = ciphertextSize();
    if (buffer_[0] != 4 || size < 16 || size > 4093)
      return State::Invalid;
    return used_ < size + 3 ? State::Incomplete : State::Ready;
  }
  const uint8_t *ciphertext() const { return buffer_ + 3; }
  size_t ciphertextSize() const {
    return (size_t(buffer_[1]) << 8) | buffer_[2];
  }
  void consume() {
    const size_t total = ciphertextSize() + 3;
    if (state() != State::Ready)
      return;
    used_ -= total;
    memmove(buffer_, buffer_ + total, used_);
  }
  void clear() { used_ = 0; }

private:
  uint8_t buffer_[Capacity]{}; // Long-lived transport object; no per-record
                               // allocation.
  size_t used_{0};
};
} // namespace esphome::tailscale::control_records
