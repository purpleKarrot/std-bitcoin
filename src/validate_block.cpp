// SPDX-License-Identifier: BSL-1.0

module;

#include <ranges>

#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

validation_status verifier::verify(bitcoin::block const& b) const
{
  if (auto status = verify(b.header()); !status.ok()) {
    return status;
  }

  auto state = BlockValidationState{};
  bool const result = CheckBlock(legacy::convert_block(b), state,
                                 legacy::convert_consensus(*_params));
  return result ? 0 : -1; // state.GetResult();
}

validation_status verifier::verify(bitcoin::block const& b,
                                   any_chain_view chain, // NOLINT
                                   std::chrono::sys_seconds now) const
{
  if (auto status = verify(b.header(), chain, now); !status.ok()) {
    return status;
  }

  auto state = BlockValidationState{};
  bool const result = ContextualCheckBlock(
    legacy::convert_block(b), state, legacy::convert_consensus(*_params),
    AnyChainView{chain | std::views::transform(legacy::convert_header)});
  return result ? 0 : -1; // state.GetResult();
}

} // namespace bitcoin
