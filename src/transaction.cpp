// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <algorithm>

#include <mp-units/framework.h>

#include <bitcoin/serdes/byte_sink.hpp>

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

inline auto calc_hash(std::uint32_t version, std::span<tx_input const> inputs,
                      std::span<tx_output const> outputs,
                      std::uint32_t locktime, bool segwit = false)
{
  auto hasher = HashWriter{};
  auto hash = std::array<std::byte, 32>{};
  serdes::encode_tx_data(hasher, version, inputs, outputs, locktime, segwit);
  std::ranges::transform(hasher.GetHash(), hash.begin(),
                         [](auto byte) { return std::byte{byte}; });
  return hash;
}

} // namespace

transaction::implementation::implementation(std::uint32_t version_,
                                            std::vector<tx_input> inputs_,
                                            std::vector<tx_output> outputs_,
                                            std::uint32_t locktime_)
  : version{version_}
  , inputs{std::move(inputs_)}
  , outputs{std::move(outputs_)}
  , locktime{locktime_}
  , is_segwit{has_witness(inputs)}
  , hash{calc_hash(version, inputs, outputs, locktime)}
  , witness_hash{calc_hash(version, inputs, outputs, locktime, is_segwit)}
{
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
  auto buf = serdes::buffered_sink<byte_sink_ref>{sink};
  serdes::encode_tx(buf, tx);
}

auto serialized_size(transaction const& tx) -> std::size_t
{
  auto sink = serdes::counting_sink{};
  serdes::encode_tx(sink, tx);
  return sink.size();
}

} // namespace bitcoin
