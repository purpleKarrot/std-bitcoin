// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block_header.hpp>

#include <cstddef>
#include <span>

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
  auto buf = serdes::buffered_sink<byte_sink_ref, 80>{std::move(sink)};
  serdes::encode_block_header(buf, header);
}

auto serialized_size(block_header const&) -> std::size_t
{
  return 80;
}

namespace detail {

void detail::block_hash_policy::operator()(block_header const& hdr,
                                           std::span<std::byte, 32> dst) const
{
  auto hasher = HashWriter{};
  serdes::encode_block_header(hasher, hdr);
  auto const hash = hasher.GetHash();
  std::ranges::transform(hash, dst.begin(),
                         [](auto byte) { return std::byte{byte}; });
}

} // namespace detail
} // namespace bitcoin
