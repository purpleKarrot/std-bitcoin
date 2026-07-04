// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/predicates.hpp>

#include <bitcoin/block.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

bool is_coinbase(txid const& t) noexcept
{
  return !static_cast<bool>(t);
}

bool is_coinbase(outpoint const& o) noexcept
{
  return is_coinbase(o.txid());
}

bool signals_rbf(tx_input const& i) noexcept
{
  return i.sequence() < 0xFFFF'FFFEu;
}

bool has_relative_locktime(tx_input const& i) noexcept
{
  return (i.sequence() & 0x8000'0000u) == 0 && i.sequence() != 0xFFFF'FFFFu;
}

bool is_unspendable(tx_output const& o) noexcept
{
  auto const bytes = as_bytes(o.script());
  return !bytes.empty() && bytes.front() == std::byte{0x6a};
}

bool is_coinbase(transaction const& t)
{
  auto inputs = t.inputs();
  return inputs.size() == 1 && is_coinbase(inputs.front().prevout());
}

bool is_segwit(transaction const& t)
{
  return std::ranges::any_of(
    t.inputs(), [](auto const& input) { return !input.witness().empty(); });
}

bool locktime_is_height(transaction const& t) noexcept
{
  return t.locktime() < 500'000'000u;
}

bool locktime_is_time(transaction const& t) noexcept
{
  return t.locktime() >= 500'000'000u;
}

bool has_coinbase(block const& b)
{
  auto transactions = b.transactions();
  return !transactions.empty() && is_coinbase(transactions.front());
}

} // namespace bitcoin
