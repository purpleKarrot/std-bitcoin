// SPDX-License-Identifier: BSL-1.0

module;

#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

validation_status verifier::verify(bitcoin::block const& b) const
{
  auto const block = legacy::convert_block(b);
  auto const params = legacy::convert_consensus(*_params);
  auto state = BlockValidationState{};
  bool const result = CheckBlock(block, state, params);
  return result ? 0 : -1; // state.GetResult();
}

// validation_status verify(bitcoin::block const& block, any_chain_view chain,
//                            std::chrono::sys_seconds now)
// {
// }

} // namespace bitcoin
