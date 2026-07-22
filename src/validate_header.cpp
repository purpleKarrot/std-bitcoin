// SPDX-License-Identifier: BSL-1.0

module;

#include <chrono>
#include <ranges>

#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

validation_status verifier::verify(block_header const& header) const
{
  auto state = BlockValidationState{};
  bool const result = CheckBlockHeader(legacy::convert_header(header), state,
                                       legacy::convert_consensus(*_params));
  return result ? 0 : -1; // state.GetResult();
}

validation_status verifier::verify(block_header const& header,
                                   any_chain_view chain,
                                   std::chrono::sys_seconds now) const
{
  // if (auto status = verify(header); !status.ok()) {
  //   return status;
  // }

  if (chain.empty()) {
    return -1;
  }

  if (header.prev_block_hash
      != block_hash{std::ranges::common_view(chain).back()}) {
    return -1;
  }

  auto state = BlockValidationState{};
  bool const result = ContextualCheckBlockHeader(
    legacy::convert_header(header), state, legacy::convert_consensus(*_params),
    AnyChainView{chain | std::views::transform(legacy::convert_header)},
    NodeClock::time_point{now.time_since_epoch()});
  return result ? 0 : -1; // state.GetResult();
}

} // namespace bitcoin
