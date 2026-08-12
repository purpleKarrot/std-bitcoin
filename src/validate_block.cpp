// SPDX-License-Identifier: BSL-1.0

module;

#include <ranges>

#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

validation_status verifier::verify(bitcoin::block const& b) const
{
  if (auto status = verify(b.header()); !status) {
    return status;
  }

  auto state = BlockValidationState{};
  auto result = CheckBlock(legacy::convert_block(b), state,
                           legacy::convert_consensus(*_params));
  return result ? validation_status{} : state.GetRejectReason();
}

validation_status verifier::verify(bitcoin::block const& b,
                                   type_erasure::any_chain_view chain, // NOLINT
                                   std::chrono::sys_seconds now) const
{
  if (auto status = verify(b.header(), chain, now); !status) {
    return status;
  }

  auto state = BlockValidationState{};
  auto result = ContextualCheckBlock(
    legacy::convert_block(b), state, legacy::convert_consensus(*_params),
    AnyChainView{chain | std::views::transform(legacy::convert_header)});
  return result ? validation_status{} : state.GetRejectReason();
}

validation_status verifier::verify(bitcoin::block const& b,
                                   type_erasure::any_chain_view chain, // NOLINT
                                   std::chrono::sys_seconds now,
                                   type_erasure::coin_index_ref coins) const
{
  return validation_status{"NOT IMPLEMENTED YET"};
}

} // namespace bitcoin
