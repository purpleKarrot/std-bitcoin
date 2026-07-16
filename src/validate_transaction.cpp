// SPDX-License-Identifier: BSL-1.0

module;

#include "consensus/tx_check.h"
#include "consensus/validation.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "script/verify_flags.h"
#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

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

} // namespace bitcoin
