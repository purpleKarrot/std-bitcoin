// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <cstddef>
#include <span>

#include <bitcoin/block.hpp>
#include <bitcoin/hash_id.hpp>
#include <bitcoin/script.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

// [bitcoin.pred.txid]
[[nodiscard]] inline auto is_coinbase(txid const& t) noexcept -> bool
{
  return !static_cast<bool>(t);
}

// [bitcoin.pred.outpoint]
[[nodiscard]] inline auto is_coinbase(outpoint const& o) noexcept -> bool
{
  return is_coinbase(o.txid());
}

// [bitcoin.pred.tx_input]
[[nodiscard]] inline auto signals_rbf(tx_input const& i) noexcept -> bool
{
  return i.sequence() < 0xFFFF'FFFEu;
}

[[nodiscard]] inline auto has_relative_locktime(tx_input const& i) noexcept
  -> bool
{
  return (i.sequence() & 0x8000'0000u) == 0 && i.sequence() != 0xFFFF'FFFFu;
}

// [bitcoin.pred.tx_output]
[[nodiscard]] inline auto is_unspendable(tx_output const& o) noexcept -> bool
{
  auto const bytes = as_bytes(o.script());
  return !bytes.empty() && bytes.front() == std::byte{0x6a};
}

// [bitcoin.pred.transaction]
[[nodiscard]] inline auto is_coinbase(transaction const& t) -> bool
{
  auto inputs = t.inputs();
  return inputs.size() == 1 && is_coinbase(inputs.front().previous_output());
}

[[nodiscard]] inline auto is_segwit(transaction const& t) -> bool
{
  return std::ranges::any_of(
    t.inputs(), [](auto const& input) { return !input.witness().empty(); });
}

[[nodiscard]] inline auto locktime_is_height(transaction const& t) noexcept
  -> bool
{
  return t.locktime() < 500'000'000u;
}

[[nodiscard]] inline auto locktime_is_time(transaction const& t) noexcept
  -> bool
{
  return t.locktime() >= 500'000'000u;
}

// [bitcoin.pred.block]
[[nodiscard]] inline auto has_coinbase(block const& b) -> bool
{
  auto transactions = b.transactions();
  return !transactions.empty() && is_coinbase(transactions.front());
}

} // namespace bitcoin
