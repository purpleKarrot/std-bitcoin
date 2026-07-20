// SPDX-License-Identifier: BSL-1.0

module;

#include <ranges>
#include <stdexcept>

#include "consensus/tx_check.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "script/verify_flags.h"
#include "validation.h"

module bitcoin;
import legacy;

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

} // namespace

namespace bitcoin {

validation_status verifier::verify(script_ref script, amount value,
                                   transaction const& tx_to,
                                   std::size_t input_index,
                                   validation_flags flags,
                                   any_prevouts_view prevouts) const
{
  assert(input_index < tx_to.inputs().size());

  // Assert that all specified flags are part of the interface before continuing
  assert(!is_set(flags, ~validation_flags::all));

  if (!is_valid_flag_combination(
        script_verify_flags::from_int(static_cast<int>(flags)))) {
    throw std::invalid_argument("Invalid flag combination");
  }

  if (is_set(flags, validation_flags::taproot)
      && (prevouts.size() != tx_to.inputs().size())) {
    throw std::invalid_argument("Spent outputs required for taproot");
  }

  auto const tx = legacy::convert_tx(tx_to);
  auto txdata = PrecomputedTransactionData{tx};
  if (is_set(flags, validation_flags::taproot)) {
    // clang-format off
    txdata.Init(tx, prevouts
      | std::views::transform(legacy::convert_txout)
      | std::ranges::to<std::vector>());
    // clang-format on
  }

  auto const checker = TransactionSignatureChecker(
    &tx, input_index, value.numerical_value_in(units::satoshi), txdata,
    MissingDataBehavior::FAIL);

  bool const result = VerifyScript(
    tx.vin[input_index].scriptSig, legacy::convert_script(script),
    &tx.vin[input_index].scriptWitness,
    script_verify_flags::from_int(static_cast<int>(flags)), checker, nullptr);

  return result ? 0 : -1;
}

} // namespace bitcoin
