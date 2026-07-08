// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/validation.hpp>

#include <stdexcept>

#include "consensus/tx_check.h"
#include "consensus/validation.h"
#include "legacy.hpp"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "script/verify_flags.h"
#include "validation.h"

namespace {

[[nodiscard]] constexpr bitcoin::validation_flags operator~(
  bitcoin::validation_flags f) noexcept
{
  return bitcoin::validation_flags(~static_cast<int>(f));
}

constexpr bool is_set(bitcoin::validation_flags f,
                      bitcoin::validation_flags flag) noexcept
{
  return (static_cast<int>(f) & static_cast<int>(flag)) != 0;
}

bool is_valid_flag_combination(script_verify_flags flags)
{
  if ((flags & SCRIPT_VERIFY_CLEANSTACK)
      && (~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS))) {
    return false;
  }
  if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) {
    return false;
  }
  return true;
}

auto as_CScript(bitcoin::script_ref script) -> CScript
{
  auto const bytes = as_bytes(script);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return {data, data + bytes.size()};
}

} // namespace

namespace bitcoin {

validation_status verifier::verify(block_header const& header) const
{
  auto params = CChainParams::Main()->GetConsensus();
  auto const bytes = as_bytes(block_hash{header});
  auto const* data = reinterpret_cast<unsigned char const*>(bytes.data());
  auto const hash = std::span{data, 32};
  bool const result = CheckProofOfWork(uint256{hash}, header.bits, params);
  return result ? 0 : -1; // BlockValidationResult::BLOCK_INVALID_HEADER
}

// validation_status verify(block_header const& header, any_chain_view chain,
//                            std::chrono::sys_seconds now)
// {
// }

validation_status verifier::verify(bitcoin::block const& b) const
{
  auto const block = legacy::convert_block(b);
  auto state = BlockValidationState{};
  auto params = CChainParams::Main()->GetConsensus();
  bool const result = CheckBlock(block, state, params);
  return result ? 0 : -1; // state.GetResult();
}

// validation_status verify(bitcoin::block const& block, any_chain_view chain,
//                            std::chrono::sys_seconds now)
// {
// }

validation_status verifier::verify(bitcoin::transaction const& tx) const
{
  auto const txn = legacy::convert_tx(tx);
  auto state = TxValidationState{};
  bool const ok = CheckTransaction(txn, state);
  return ok ? 0 : -1; // state.GetResult();
}

// validation_status verify(
//   bitcoin::transaction const& tx, any_chain_view chain)
// {
// }

validation_status verifier::verify(script_ref script, amount value,
                                   transaction const& tx_to,
                                   std::size_t input_index,
                                   validation_flags flags,
                                   any_prevouts_view prevouts) const
{
  // Assert that all specified flags are part of the interface before continuing
  assert(!is_set(flags, ~validation_flags::all));

  if (!is_valid_flag_combination(
        script_verify_flags::from_int(static_cast<int>(flags)))) {
    throw std::invalid_argument("Invalid flag combination");
  }

  if (is_set(flags, validation_flags::taproot) && prevouts.empty()) {
    throw std::invalid_argument("Spent outputs required for taproot");
  }

  CTransaction const& tx = legacy::convert_tx(tx_to);
  std::vector<CTxOut> spent_outputs;
  if (!prevouts.empty()) {
    assert(prevouts.size() == tx.vin.size());
    spent_outputs.reserve(prevouts.size());
    for (auto const& elem : prevouts) {
      spent_outputs.push_back(legacy::convert_txout(elem));
    }
  }

  assert(input_index < tx.vin.size());
  PrecomputedTransactionData txdata{tx};

  if (!spent_outputs.empty() && is_set(flags, validation_flags::taproot)) {
    txdata.Init(tx, std::move(spent_outputs));
  }

  auto const checker = TransactionSignatureChecker(
    &tx, input_index, value.numerical_value_in(units::satoshi), txdata,
    MissingDataBehavior::FAIL);

  bool const result = VerifyScript(
    tx.vin[input_index].scriptSig, as_CScript(script),
    &tx.vin[input_index].scriptWitness,
    script_verify_flags::from_int(static_cast<int>(flags)), checker, nullptr);

  return result ? 0 : -1;
}

} // namespace bitcoin
