// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cassert>

#include <bitcoin/block.hpp>
#include <bitcoin/transaction.hpp>

#include "primitives/block.h"
#include "primitives/transaction.h"

namespace bitcoin {
namespace detail {

struct block_data : CBlock
{
};

struct transaction_data : CTransaction
{
  using CTransaction::CTransaction;
};

} // namespace detail

struct _impl_access
{
  static auto get(block const& tx) -> CBlock const&
  {
    assert(tx._data != nullptr);
    return *tx._data;
  }

  static auto get(transaction const& tx) -> CTransaction const&
  {
    assert(tx._data != nullptr);
    return *tx._data;
  }

  static auto get(tx_output const& out) -> CTxOut const&
  {
    assert(out._data != nullptr);
    return out._data->vout[out._index];
  }
};

} // namespace bitcoin
