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
  auto result = CheckBlockHeader(legacy::convert_header(header), state,
                                 legacy::convert_consensus(*_params));
  return result ? validation_status{} : state.GetRejectReason();
}

validation_status verifier::verify(block_header const& header,
                                   type_erasure::any_chain_view chain,
                                   std::chrono::sys_seconds now) const
{
  // if (auto status = verify(header); !status.ok()) {
  //   return status;
  // }

  if (chain.empty()) {
    return validation_status{"parent not found"};
  }

  if (header.prev_block_hash
      != block_hash{std::ranges::common_view(chain).back()}) {
    return validation_status{"parent not found"};
  }

  auto state = BlockValidationState{};
  auto result = ContextualCheckBlockHeader(
    legacy::convert_header(header), state, legacy::convert_consensus(*_params),
    AnyChainView{chain | std::views::transform(legacy::convert_header)},
    NodeClock::time_point{now.time_since_epoch()});
  return result ? validation_status{} : state.GetRejectReason();
}

} // namespace bitcoin
