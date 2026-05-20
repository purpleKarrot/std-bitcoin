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
  header._version = core_header.nVersion;
  header._prev_block_hash = bitcoin::block_hash{std::span<std::byte const, 32>{
    reinterpret_cast<std::byte const*>(core_header.hashPrevBlock.data()), 32}};
  header._merkle_root = bitcoin::hash256{std::span<std::byte const, 32>{
    reinterpret_cast<std::byte const*>(core_header.hashMerkleRoot.data()), 32}};
  header._time = core_header.nTime;
  header._bits = core_header.nBits;
  header._nonce = core_header.nNonce;
  return header;
}

void serialize(block_header const& header, serialization::byte_sink_ref sink)
{
  ::Serialize(sink, header._version);
  ::Serialize(sink, as_bytes(header._prev_block_hash));
  ::Serialize(sink, as_bytes(header._merkle_root));
  ::Serialize(sink, header._time);
  ::Serialize(sink, header._bits);
  ::Serialize(sink, header._nonce);
}

auto serialized_size(block_header const&) -> std::size_t
{
  return 80;
}

auto block_header::hash() const noexcept -> bitcoin::block_hash
{
  auto hasher = HashWriter{};
  serialize(*this, hasher);
  auto const hash = hasher.GetHash();
  return bitcoin::block_hash{std::span<std::byte const, 32>{
    reinterpret_cast<std::byte const*>(hash.begin()), 32}};
}

} // namespace bitcoin
