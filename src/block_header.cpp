// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block_header.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include <bitcoin/hash_id.hpp>
#include <bitcoin/serdes/byte_sink.hpp>

#include "hash.h"
#include "serdes_decode.hpp"
#include "serdes_encode.hpp"

namespace bitcoin {

auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>
{
  auto decoder = serdes::decoder{serdes::span_source{raw}};
  auto header = serdes::decode_block_header(decoder);
  if (!decoder.good() || !decoder.source().empty()) {
    return std::nullopt;
  }
  return header;
}

void detail::serialize(block_header const& header, byte_sink_ref sink)
{
  auto buf = serdes::buffered_sink<byte_sink_ref, 80>{sink};
  serdes::encode_block_header(buf, header);
}

auto serialized_size(block_header const& /*unused*/) -> std::size_t
{
  return 80;
}

namespace detail {

auto detail::block_hash_policy::operator()(block_header const& hdr) const
  -> std::array<std::byte, 32>
{
  auto hasher = HashWriter{};
  auto hash = std::array<std::byte, 32>{};
  serdes::encode_block_header(hasher, hdr);
  std::ranges::transform(hasher.GetHash(), hash.begin(),
                         [](auto byte) { return std::byte{byte}; });
  return hash;
}

} // namespace detail
} // namespace bitcoin
