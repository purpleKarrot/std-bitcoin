// SPDX-License-Identifier: BSL-1.0

module;

#include <algorithm>
#include <chrono>
#include <ranges>
#include <vector>

#include "pow.h"

module bitcoin;
import legacy;

namespace bitcoin {
namespace {

auto median_time_past(chain_view auto chain)
{
  assert(!chain.empty());
  auto times = chain
    | std::views::transform(&block_header::time)
    | std::views::reverse
    | std::views::take(11)
    | std::ranges::to<std::vector>();
  auto const middle = times.begin() + (times.size() / 2);
  std::ranges::nth_element(times, middle);
  return *middle;
}

} // namespace

validation_status verifier::verify(block_header const& header) const
{
  auto const hash = legacy::convert_uint256(block_hash{header});
  auto const params = legacy::convert_consensus(*_params);
  bool const result = CheckProofOfWork(hash, header.bits, params);
  return result ? 0 : -1; // BlockValidationResult::BLOCK_INVALID_HEADER
}

validation_status verifier::verify(block_header const& header,
                                   any_chain_view chain,
                                   std::chrono::sys_seconds now) const
{
  if (chain.empty()) {
    return -1;
  }

  // A header MUST reference the hash of a valid parent block.
  if (header.prev_block_hash != block_hash{chain[chain.size() - 1]}) {
    return -1;
  }

  // A header's hash MUST achieve its own proof-of-work target.
  // TODO

  // A header's proof-of-work target MUST satisfy the difficulty adjustment
  // TODO

  // A header timestamp MUST be strictly greater than the median of its 11
  // ancestors' timestamps.
  if (header.time <= median_time_past(chain)) {
    return -1;
  }

  // A header timestamp MUST be less than or equal to network-adjusted time plus
  // 2 hours.
  if (header.time > now + std::chrono::hours(2)) {
    return -1;
  }

  // A header's version number MUST NOT have been retired by any activated
  // soft fork.
  // TODO

  return -1;
}

} // namespace bitcoin
