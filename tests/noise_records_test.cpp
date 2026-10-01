#include "noise_records.h"
#include <cassert>
#include <vector>
int main() {
  using namespace esphome::tailscale::control_records;
  const std::vector<uint8_t> record{4, 0, 16, 1,  2,  3,  4,  5,  6, 7,
                                    8, 9, 10, 11, 12, 13, 14, 15, 16};
  for (size_t split = 0; split <= record.size(); ++split) {
    Records r;
    assert(r.append(record.data(), split));
    assert(r.state() == (split == record.size() ? Records::State::Ready
                                                : Records::State::Incomplete));
    assert(r.append(record.data() + split, record.size() - split));
    assert(r.state() == Records::State::Ready);
    assert(r.ciphertextSize() == 16 && r.ciphertext()[15] == 16);
    r.consume();
    assert(r.state() == Records::State::Incomplete);
  }
  Records combined;
  assert(combined.append(record.data(), record.size()));
  assert(combined.append(record.data(), record.size()));
  combined.consume();
  assert(combined.state() == Records::State::Ready);
  combined.consume();
  assert(combined.state() == Records::State::Incomplete);
  for (auto bytes : std::vector<std::vector<uint8_t>>{
           {3, 0, 16}, {4, 0, 15}, {4, 16, 0}, {4, 255, 255}}) {
    Records r;
    assert(r.append(bytes.data(), bytes.size()));
    assert(r.state() == Records::State::Invalid);
  }
  Records limit;
  std::vector<uint8_t> huge(Records::Capacity + 1, 0);
  assert(!limit.append(huge.data(), huge.size()));
  assert(limit.append(huge.data(), Records::Capacity));
  assert(!limit.append(huge.data(), 1));
  limit.clear();
  assert(limit.state() == Records::State::Incomplete);
}
