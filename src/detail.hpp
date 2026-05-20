// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "primitives/block.h"
#include "primitives/transaction.h"

namespace bitcoin::detail {

struct block_data : CBlock
{
};

struct transaction_data : CTransaction
{
  using CTransaction::CTransaction;
};

} // namespace bitcoin::detail
