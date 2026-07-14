// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block.hpp>

#include <mp-units/framework.h>

#include <bitcoin/serdes/byte_sink.hpp>

#include "serdes_decode.hpp"
#include "serdes_encode.hpp"

namespace bitcoin {

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
  auto buf = serdes::buffered_sink<byte_sink_ref>{sink};
  serdes::encode_block(buf, b);
}

auto serialized_size(block const& b) -> std::size_t
{
  auto sink = serdes::counting_sink{};
  serdes::encode_block(sink, b);
  return sink.size();
}

} // namespace bitcoin
