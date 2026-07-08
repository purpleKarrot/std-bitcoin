// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block.hpp>

#include <cassert>
#include <ranges>
#include <vector>

#include "serdes_decode.hpp"
#include "serdes_encode.hpp"

namespace bitcoin {

block::block() = default;

block::block(block_header header, std::vector<transaction> transactions)
  : _header{header}
  , _transactions{std::move(transactions)}
{
}

auto block::header() const noexcept -> block_header const&
{
  return _header;
}

auto block::transactions() const -> transaction_view
{
  return _transactions;
}

bool operator==(block const& lhs, block const& rhs) noexcept = default;

auto parse_block(std::span<std::byte const> raw) -> std::optional<block>
{
  auto decoder = serdes::decoder{serdes::span_source{raw}};
  auto b = serdes::decode_block(decoder);
  if (!decoder.good() || !decoder.source().empty()) {
    return std::nullopt;
  }
  return b;
}

void detail::serialize(block const& b, byte_sink_ref sink)
{
  auto buf = serdes::buffered_sink<byte_sink_ref>{std::move(sink)};
  serdes::encode_block(buf, b);
}

auto serialized_size(block const& b) -> std::size_t
{
  auto sink = serdes::counting_sink{};
  serdes::encode_block(sink, b);
  return sink.size();
}

void detail::block_hash_policy::operator()(block const& b,
                                           std::span<std::byte, 32> dst) const
{
  operator()(b.header(), dst);
}

} // namespace bitcoin
