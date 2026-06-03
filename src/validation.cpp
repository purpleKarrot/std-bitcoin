// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/validation.hpp>

#include "consensus/tx_check.h"
#include "detail.hpp"
#include "pow.h"
#include "validation.h"

namespace bitcoin {

verification_status verify(block_header const& header)
{
  auto params = CChainParams::Main()->GetConsensus();
  auto const bytes = as_bytes(header.hash());
  auto const* data = reinterpret_cast<unsigned char const*>(bytes.data());
  auto const hash = std::span{data, 32};
  bool const result = CheckProofOfWork(uint256{hash}, header.bits(), params);
  return result ? 0 : -1; // BlockValidationResult::BLOCK_INVALID_HEADER
}

// verification_status verify(block_header const& header, any_chain_view chain,
//                            std::chrono::sys_seconds now)
// {
// }

verification_status verify(bitcoin::block const& block)
{
  auto state = BlockValidationState{};
  auto params = CChainParams::Main()->GetConsensus();
  bool const result = CheckBlock(_impl_access::get(block), state, params);
  return result ? 0 : -1; // state.GetResult();
}

// verification_status verify(bitcoin::block const& block, any_chain_view chain,
//                            std::chrono::sys_seconds now)
// {
// }

verification_status verify(bitcoin::transaction const& tx)
{
  auto state = TxValidationState{};
  bool const ok = CheckTransaction(_impl_access::get(tx), state);
  return ok ? 0 : -1; // state.GetResult();
}

// verification_status verify(
//   bitcoin::transaction const& tx, any_chain_view chain)
// {
// }

} // namespace bitcoin

std::format_context::iterator
std::formatter<bitcoin::verification_status>::format(
  bitcoin::verification_status status, std::format_context& ctx) const
{
  auto str = status ? "OK" : "NOT OK";
  return std::formatter<string_view>::format(str, ctx);
}
