// SPDX-License-Identifier: BSL-1.0

module;

#include <cstddef>

export module bitcoin:predicates;

import :vocabulary;

export namespace bitcoin {

// [bitcoin.pred.txid]
[[nodiscard]] bool is_coinbase(txid const& t) noexcept
{
  return !static_cast<bool>(t);
}

// [bitcoin.pred.outpoint]
[[nodiscard]] bool is_coinbase(outpoint const& o) noexcept
{
  return is_coinbase(o.txid());
}

// [bitcoin.pred.tx_input]
[[nodiscard]] bool signals_rbf(tx_input const& i) noexcept
{
  return i.sequence() < 0xFFFF'FFFEu;
}

[[nodiscard]] bool has_relative_locktime(tx_input const& i) noexcept
{
  return (i.sequence() & 0x8000'0000u) == 0 && i.sequence() != 0xFFFF'FFFFu;
}

// [bitcoin.pred.tx_output]
[[nodiscard]] bool is_unspendable(tx_output const& o) noexcept
{
  auto const bytes = as_bytes(o.script());
  return !bytes.empty() && bytes.front() == std::byte{0x6a};
}

// [bitcoin.pred.transaction]
[[nodiscard]] bool is_coinbase(transaction const& t)
{
  auto inputs = t.inputs();
  return inputs.size() == 1 && is_coinbase(inputs.front().prevout());
}

[[nodiscard]] bool is_segwit(transaction const& t)
{
  return t._impl->is_segwit;
}

[[nodiscard]] bool locktime_is_height(transaction const& t) noexcept
{
  return t.locktime() < 500'000'000u;
}

[[nodiscard]] bool locktime_is_time(transaction const& t) noexcept
{
  return t.locktime() >= 500'000'000u;
}

// [bitcoin.pred.block]
[[nodiscard]] bool has_coinbase(block const& b)
{
  auto transactions = b.transactions();
  return !transactions.empty() && is_coinbase(transactions.front());
}

} // namespace bitcoin
