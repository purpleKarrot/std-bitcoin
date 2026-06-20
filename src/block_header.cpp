// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block_header.hpp>

#include <cstddef>
#include <span>

#include "hash.h"
#include "primitives/block.h"
#include "serialize.h"
#include "streams.h"

namespace bitcoin {

auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>
{
  auto stream = DataStream{raw};
  auto core_header = CBlockHeader{};
  stream >> core_header;
  if (!stream.empty()) {
    return std::nullopt;
  }

  auto header = block_header{};
  header.version = core_header.nVersion;
  header.prev_block_hash = bitcoin::block_hash{std::span<std::byte const, 32>{
    reinterpret_cast<std::byte const*>(core_header.hashPrevBlock.data()), 32}};
  header.merkle_root = bitcoin::hash256{std::span<std::byte const, 32>{
    reinterpret_cast<std::byte const*>(core_header.hashMerkleRoot.data()), 32}};
  header.time =
    std::chrono::sys_seconds{std::chrono::seconds{core_header.nTime}};
  header.bits = core_header.nBits;
  header.nonce = core_header.nNonce;
  return header;
}

void serialize(block_header const& header, serialization::byte_sink_ref sink)
{
  ::Serialize(sink, header.version);
  ::Serialize(sink, as_bytes(header.prev_block_hash));
  ::Serialize(sink, as_bytes(header.merkle_root));
  ::Serialize(sink, uint32_t(header.time.time_since_epoch().count()));
  ::Serialize(sink, header.bits);
  ::Serialize(sink, header.nonce);
}

auto serialized_size(block_header const&) -> std::size_t
{
  return 80;
}

namespace detail {

void detail::block_hash_tag::operator()(block_header const& hdr,
                                        std::span<std::byte, 32> dst) const
{
  auto hasher = HashWriter{};
  serialize(hdr, hasher);
  auto const hash = hasher.GetHash();
  std::ranges::transform(hash, dst.begin(),
                         [](auto byte) { return std::byte{byte}; });
}

} // namespace detail
} // namespace bitcoin
