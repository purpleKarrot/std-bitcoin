// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <cassert>
#include <ranges>
#include <utility>

#include "detail.hpp"
#include "primitives/transaction.h"
#include "streams.h"

namespace bitcoin {
namespace {

auto as_CScript(bitcoin::script_ref script) -> CScript
{
  auto const bytes = as_bytes(script);
  auto const* data = reinterpret_cast<uint8_t const*>(bytes.data());
  return {data, data + bytes.size()};
}

} // namespace

tx_input::tx_input(std::shared_ptr<detail::transaction_data const> data,
                   std::size_t index) noexcept
  : _data{std::move(data)}
  , _index{index}
{
}

tx_output::tx_output(std::shared_ptr<detail::transaction_data const> data,
                     std::size_t index) noexcept
  : _data{std::move(data)}
  , _index{index}
{
}

transaction::transaction(std::shared_ptr<detail::transaction_data const> data)
  : _data(std::move(data))
{
}

auto tx_input::prevout() const noexcept -> outpoint
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

auto tx_input::witness() const -> witness_view
{
  assert(_data != nullptr);
  assert(_index < _data->vin.size());
  constexpr auto conv = [](auto const& v) { return as_bytes(std::span{v}); };
  return _data->vin[_index].scriptWitness.stack | std::views::transform(conv);
}

bool operator==(tx_input const& lhs, tx_input const& rhs) noexcept
{
  if (lhs._data == rhs._data && lhs._index == rhs._index) {
    return true;
  }

  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }

  assert(lhs._index < lhs._data->vin.size());
  assert(rhs._index < rhs._data->vin.size());
  return (lhs._data->vin[lhs._index] == rhs._data->vin[rhs._index])
    && (lhs._data->vin[lhs._index].scriptWitness.stack
        == rhs._data->vin[rhs._index].scriptWitness.stack);
}

tx_output::tx_output(amount value, script_ref script)
  : _index{0}
{
  auto mut = CMutableTransaction{};
  mut.vout.emplace_back(value.numerical_value_in(units::satoshi),
                        as_CScript(script));
  _data = std::make_shared<detail::transaction_data>(std::move(mut));
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

bool operator==(tx_output const& lhs, tx_output const& rhs) noexcept
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

void detail::txid_policy::operator()(bitcoin::transaction const& tx,
                                     std::span<std::byte, 32ul> dst) const
{
  assert(tx._data != nullptr);
  auto const& hash = tx._data->GetHash();
  std::ranges::copy(hash, dst.begin());
}

void detail::wtxid_policy::operator()(bitcoin::transaction const& tx,
                                      std::span<std::byte, 32ul> dst) const
{
  assert(tx._data != nullptr);
  auto const& hash = tx._data->GetWitnessHash();
  std::ranges::copy(hash, dst.begin());
}

auto transaction::version() const noexcept -> std::uint32_t
{
  assert(_data != nullptr);
  return _data->version;
}

auto transaction::locktime() const noexcept -> std::uint32_t
{
  assert(_data != nullptr);
  return _data->nLockTime;
}

auto transaction::inputs() const -> input_view
{
  assert(_data != nullptr);
  auto indices = std::views::iota(std::size_t{0}, _data->vin.size());
  return std::views::transform(indices, [data = _data](std::size_t index) {
    return tx_input{data, index};
  });
}

auto transaction::outputs() const -> output_view
{
  assert(_data != nullptr);
  auto indices = std::views::iota(std::size_t{0}, _data->vout.size());
  return std::views::transform(indices, [data = _data](std::size_t index) {
    return tx_output{data, index};
  });
}

bool operator==(transaction const& lhs, transaction const& rhs) noexcept
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

void detail::serialize(transaction const& tx, byte_sink_ref sink)
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
