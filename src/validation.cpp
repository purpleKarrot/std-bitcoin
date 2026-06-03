// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/validation.hpp>

#include <stdexcept>

#include "consensus/tx_check.h"
#include "consensus/validation.h"
#include "detail.hpp"
#include "pow.h"
#include "script/interpreter.h"
#include "script/verify_flags.h"
#include "validation.h"

namespace {

[[nodiscard]] constexpr bitcoin::verification_flags operator&(
  bitcoin::verification_flags l, bitcoin::verification_flags r) noexcept
{
  return bitcoin::verification_flags(static_cast<int>(l) & static_cast<int>(r));
}

[[nodiscard]] constexpr bitcoin::verification_flags operator~(
  bitcoin::verification_flags f) noexcept
{
  return bitcoin::verification_flags(~static_cast<int>(f));
}

constexpr bool is_set(bitcoin::verification_flags f,
                      bitcoin::verification_flags flag) noexcept
{
  return (f & flag) != bitcoin::verification_flags::none;
}

bool is_valid_flag_combination(script_verify_flags flags)
{
  if (flags & SCRIPT_VERIFY_CLEANSTACK &&
      ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) {
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

verification_status verify(
  script_ref script, amount value, transaction const& tx_to,
  std::size_t input_index, verification_flags flags,
  beman::any_view::any_view<tx_output const,
                            beman::any_view::any_view_options::random_access |
                              beman::any_view::any_view_options::sized>
    spent_outputs_)
{
  // Assert that all specified flags are part of the interface before continuing
  assert(!is_set(flags, ~verification_flags::all));

  if (!is_valid_flag_combination(
        script_verify_flags::from_int(static_cast<int>(flags)))) {
    throw std::invalid_argument("Invalid flag combination");
  }

  if (is_set(flags, verification_flags::taproot) && spent_outputs_.empty()) {
    throw std::invalid_argument("Spent outputs required for taproot");
  }

  CTransaction const& tx = _impl_access::get(tx_to);
  std::vector<CTxOut> spent_outputs;
  if (!spent_outputs_.empty()) {
    assert(spent_outputs_.size() == tx.vin.size());
    spent_outputs.reserve(spent_outputs_.size());
    for (auto const& elem : spent_outputs_) {
      spent_outputs.push_back(_impl_access::get(elem));
    }
  }

  assert(input_index < tx.vin.size());
  PrecomputedTransactionData txdata{tx};

  if (!spent_outputs.empty() && is_set(flags, verification_flags::taproot)) {
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

std::format_context::iterator
std::formatter<bitcoin::verification_flags>::format(
  bitcoin::verification_flags flags, std::format_context& ctx) const
{
  if (flags == bitcoin::verification_flags::none) {
    return std::formatter<string_view>::format("NONE", ctx);
  }

  using bitcoin::verification_flags;
  static constexpr auto entries = std::array{
    std::pair{verification_flags::p2sh, "P2SH"},
    std::pair{verification_flags::dersig, "DERSIG"},
    std::pair{verification_flags::nulldummy, "NULLDUMMY"},
    std::pair{verification_flags::checklocktimeverify, "CHECKLOCKTIMEVERIFY"},
    std::pair{verification_flags::checksequenceverify, "CHECKSEQUENCEVERIFY"},
    std::pair{verification_flags::witness, "WITNESS"},
    std::pair{verification_flags::taproot, "TAPROOT"},
  };

  bool empty = true;
  for (auto const& [f, name] : entries) {
    if (is_set(flags, f)) {
      if (!empty) {
        *ctx.out()++ = '|';
      }
      std::formatter<string_view>::format(name, ctx);
      empty = false;
    }
  }

  return ctx.out();
}

std::format_context::iterator
std::formatter<bitcoin::verification_status>::format(
  bitcoin::verification_status status, std::format_context& ctx) const
{
  auto str = status ? "OK" : "NOT OK";
  return std::formatter<string_view>::format(str, ctx);
}
