// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <cassert>
#include <ranges>
#include <utility>

#include "hash.h"
#include "serdes_decode.hpp"
#include "serdes_encode.hpp"

namespace bitcoin {
namespace {

inline bool has_witness(std::vector<tx_input> const& inputs)
{
  return std::ranges::any_of(
    inputs, [](tx_input const& input) { return !input.witness().empty(); });
}

} // namespace

tx_input::tx_input(bitcoin::outpoint prevout, bitcoin::script script,
                   std::uint32_t sequence,
                   std::vector<std::vector<std::byte>> witness)
  : _prevout{std::move(prevout)}
  , _script{std::move(script)}
  , _sequence{sequence}
  , _witness{std::move(witness)}
{
}

tx_input::tx_input(tx_input&& other,
                   std::vector<std::vector<std::byte>> witness)
  : _prevout{std::move(other._prevout)}
  , _script{std::move(other._script)}
  , _sequence{other._sequence}
  , _witness{std::move(witness)}
{
}

tx_output::tx_output(bitcoin::amount value, bitcoin::script script)
  : _value{value}
  , _script{std::move(script)}
{
}

transaction::transaction(std::uint32_t version, std::vector<tx_input> inputs,
                         std::vector<tx_output> outputs, std::uint32_t locktime)
  : _version{version}
  , _inputs{std::move(inputs)}
  , _outputs{std::move(outputs)}
  , _locktime{locktime}
  , _has_witness{has_witness(_inputs)}
{
  {
    auto hasher = HashWriter{};
    serdes::encode_tx(serdes::witness::disallow)(hasher, *this);
    auto const hash = hasher.GetHash();
    std::ranges::transform(hash, _hash.begin(),
                           [](auto byte) { return std::byte{byte}; });
  }
  {
    auto hasher = HashWriter{};
    serdes::encode_tx(serdes::witness::allow)(hasher, *this);
    auto const hash = hasher.GetHash();
    std::ranges::transform(hash, _witness_hash.begin(),
                           [](auto byte) { return std::byte{byte}; });
  }
}

auto tx_input::prevout() const noexcept -> outpoint
{
  return _prevout;
}

auto tx_input::script() const noexcept -> script_ref
{
  return _script;
}

auto tx_input::sequence() const noexcept -> std::uint32_t
{
  return _sequence;
}

auto tx_input::witness() const -> witness_view
{
  return _witness;
}

bool operator==(tx_input const& lhs, tx_input const& rhs) noexcept = default;

auto tx_output::value() const noexcept -> amount
{
  return _value;
}

auto tx_output::script() const noexcept -> script_ref
{
  return _script;
}

bool operator==(tx_output const& lhs, tx_output const& rhs) noexcept = default;

transaction::transaction() = default;

void detail::txid_policy::operator()(bitcoin::transaction const& tx,
                                     std::span<std::byte, 32ul> dst) const
{
  std::ranges::copy(tx._hash, dst.begin());
}

void detail::wtxid_policy::operator()(bitcoin::transaction const& tx,
                                      std::span<std::byte, 32ul> dst) const
{
  std::ranges::copy(tx._witness_hash, dst.begin());
}

auto transaction::version() const noexcept -> std::uint32_t
{
  return _version;
}

auto transaction::locktime() const noexcept -> std::uint32_t
{
  return _locktime;
}

auto transaction::inputs() const -> input_view
{
  return _inputs;
}

auto transaction::outputs() const -> output_view
{
  return _outputs;
}

bool operator==(transaction const& lhs,
                transaction const& rhs) noexcept = default;

auto parse_transaction(std::span<std::byte const> raw)
  -> std::optional<transaction>
{
  auto decoder = serdes::decoder{serdes::span_source{raw}};
  auto tx = serdes::decode_tx(decoder);
  if (!decoder.good() || !decoder.source().empty()) {
    return std::nullopt;
  }
  return tx;
}

void detail::serialize(transaction const& tx, byte_sink_ref sink)
{
  auto buf = serdes::buffered_sink<byte_sink_ref>{std::move(sink)};
  serdes::encode_tx(serdes::witness::allow)(buf, tx);
}

auto serialized_size(transaction const& tx) -> std::size_t
{
  auto sink = serdes::counting_sink{};
  serdes::encode_tx(serdes::witness::allow)(sink, tx);
  return sink.size();
}

} // namespace bitcoin
