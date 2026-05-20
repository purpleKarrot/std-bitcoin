// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <cassert>
#include <ranges>

#include "primitives/transaction.h"
#include "streams.h"

namespace bitcoin::detail {

struct transaction_data : CTransaction
{
  using CTransaction::CTransaction;
};

} // namespace bitcoin::detail

namespace bitcoin {

auto tx_input::previous_output() const noexcept -> outpoint
{
  assert(_data != nullptr);
  assert(_index < _data->vin.size());
  auto const& prevout = _data->vin[_index].prevout;
  auto const data = std::span<std::byte const, 32>{prevout.hash.data(), 32};
  return {bitcoin::txid{data}, prevout.n};
}

auto tx_input::script() const noexcept -> script_ref
{
  assert(_data != nullptr);
  assert(_index < _data->vin.size());
  return script_ref{as_bytes(std::span{_data->vin[_index].scriptSig})};
}

auto tx_input::sequence() const noexcept -> std::uint32_t
{
  assert(_data != nullptr);
  assert(_index < _data->vin.size());
  return _data->vin[_index].nSequence;
}

auto tx_input::witness() const noexcept -> witness_view
{
  assert(_data != nullptr);
  assert(_index < _data->vin.size());
  constexpr auto conv = [](auto const& v) { return as_bytes(std::span{v}); };
  return _data->vin[_index].scriptWitness.stack | std::views::transform(conv);
}

auto operator==(tx_input const& lhs, tx_input const& rhs) noexcept -> bool
{
  if (lhs._data == rhs._data && lhs._index == rhs._index) {
    return true;
  }

  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }

  assert(lhs._index < lhs._data->vin.size());
  assert(rhs._index < rhs._data->vin.size());
  return lhs._data->vin[lhs._index] == rhs._data->vin[rhs._index] &&
    lhs._data->vin[lhs._index].scriptWitness.stack ==
    rhs._data->vin[rhs._index].scriptWitness.stack;
}

auto tx_output::value() const noexcept -> amount
{
  assert(_data != nullptr);
  assert(_index < _data->vout.size());
  return _data->vout[_index].nValue * units::satoshi;
}

auto tx_output::script() const noexcept -> script_ref
{
  assert(_data != nullptr);
  assert(_index < _data->vout.size());
  return script_ref{as_bytes(std::span{_data->vout[_index].scriptPubKey})};
}

auto operator==(tx_output const& lhs, tx_output const& rhs) noexcept -> bool
{
  if (lhs._data == rhs._data && lhs._index == rhs._index) {
    return true;
  }

  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }

  assert(lhs._index < lhs._data->vout.size());
  assert(rhs._index < rhs._data->vout.size());
  return lhs._data->vout[lhs._index] == rhs._data->vout[rhs._index];
}

transaction::transaction()
  : _data(std::make_shared<detail::transaction_data>(CMutableTransaction{}))
{
}

auto transaction::id() const noexcept -> txid
{
  assert(_data != nullptr);
  auto const& hash = _data->GetHash();
  return txid{std::span<std::byte const, 32>{hash.data(), 32}};
}

auto transaction::witness_id() const noexcept -> wtxid
{
  assert(_data != nullptr);
  auto const& hash = _data->GetWitnessHash();
  return wtxid{std::span<std::byte const, 32>{hash.data(), 32}};
}

auto transaction::version() const noexcept -> std::int32_t
{
  assert(_data != nullptr);
  return _data->version;
}

auto transaction::locktime() const noexcept -> std::uint32_t
{
  assert(_data != nullptr);
  return _data->nLockTime;
}

auto transaction::inputs() const noexcept -> input_view
{
  assert(_data != nullptr);
  auto indices = std::views::iota(std::size_t{0}, _data->vin.size());
  return std::views::transform(indices, [data = _data](std::size_t index) {
    return tx_input{data, index};
  });
}

auto transaction::outputs() const noexcept -> output_view
{
  assert(_data != nullptr);
  auto indices = std::views::iota(std::size_t{0}, _data->vout.size());
  return std::views::transform(indices, [data = _data](std::size_t index) {
    return tx_output{data, index};
  });
}

auto operator==(transaction const& lhs, transaction const& rhs) noexcept -> bool
{
  if (lhs._data == rhs._data) {
    return true;
  }

  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }

  return *lhs._data == *rhs._data;
}

auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>
{
  try {
    auto stream = DataStream{raw};
    auto txdata = std::make_shared<detail::transaction_data>(
      deserialize, TX_WITH_WITNESS, stream);
    if (!stream.empty()) {
      return std::nullopt;
    }
    return transaction{std::move(txdata)};
  }
  catch (std::ios_base::failure const&) {
    return std::nullopt;
  }
}

void serialize(transaction const& tx, serialization::byte_sink_ref sink)
{
  assert(tx._data != nullptr);
  ::Serialize(sink, TX_WITH_WITNESS(*tx._data));
}

auto serialized_size(transaction const& tx) -> std::size_t
{
  assert(tx._data != nullptr);
  return ::GetSerializeSize(TX_WITH_WITNESS(*tx._data));
}

} // namespace bitcoin
