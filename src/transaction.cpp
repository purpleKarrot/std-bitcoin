// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <cassert>
#include <ranges>
#include <utility>

#include "hash.h"
#include "serdes_decode.hpp"
#include "serdes_encode.hpp"

namespace bitcoin {
namespace detail {
namespace {

inline bool has_witness(std::vector<tx_input> const& inputs)
{
  return std::ranges::any_of(
    inputs, [](tx_input const& input) { return !input.witness().empty(); });
}

} // namespace
} // namespace detail

transaction::transaction(std::uint32_t version, std::vector<tx_input> inputs,
                         std::vector<tx_output> outputs, std::uint32_t locktime)
  : _impl{std::in_place, version, std::move(inputs), std::move(outputs),
          locktime}
{
  _impl.modify([this](implementation& impl) {
    {
      auto hasher = HashWriter{};
      serdes::encode_tx(serdes::witness::disallow)(hasher, *this);
      auto const hash_ = hasher.GetHash();
      std::ranges::transform(hash_, impl.hash.begin(),
                             [](auto byte) { return std::byte{byte}; });
    }
    {
      auto hasher = HashWriter{};
      serdes::encode_tx(serdes::witness::allow)(hasher, *this);
      auto const hash_ = hasher.GetHash();
      std::ranges::transform(hash_, impl.witness_hash.begin(),
                             [](auto byte) { return std::byte{byte}; });
    }
  });
}

transaction::implementation::implementation(std::uint32_t version_,
                                            std::vector<tx_input> inputs_,
                                            std::vector<tx_output> outputs_,
                                            std::uint32_t locktime_)
  : version{version_}
  , inputs{std::move(inputs_)}
  , outputs{std::move(outputs_)}
  , locktime{locktime_}
  , has_witness{detail::has_witness(inputs)}
{
}

void detail::txid_policy::operator()(bitcoin::transaction const& tx,
                                     std::span<std::byte, 32ul> dst) const
{
  std::ranges::copy(tx._impl->hash, dst.begin());
}

void detail::wtxid_policy::operator()(bitcoin::transaction const& tx,
                                      std::span<std::byte, 32ul> dst) const
{
  std::ranges::copy(tx._impl->witness_hash, dst.begin());
}

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
