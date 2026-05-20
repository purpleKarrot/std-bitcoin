// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/block.hpp>
#include <bitcoin/block_header.hpp>
#include <bitcoin/predicates.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

namespace {

constexpr auto byte(unsigned value) noexcept -> std::byte
{
  return std::byte{static_cast<std::uint8_t>(value)};
}

auto hex_nibble(char c) -> unsigned
{
  if (c >= '0' && c <= '9') {
    return static_cast<unsigned>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<unsigned>(c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return static_cast<unsigned>(c - 'A' + 10);
  }
  throw std::invalid_argument{"invalid hex digit"};
}

auto hex_bytes(std::string_view hex) -> std::vector<std::byte>
{
  if (hex.size() % 2 != 0) {
    throw std::invalid_argument{"hex string must have even length"};
  }

  auto bytes = std::vector<std::byte>{};
  bytes.reserve(hex.size() / 2);
  for (std::size_t i = 0; i < hex.size(); i += 2) {
    auto const value = (hex_nibble(hex[i]) << 4U) | hex_nibble(hex[i + 1]);
    bytes.push_back(byte(value));
  }
  return bytes;
}

struct vector_sink
{
  void write(std::span<std::byte const> chunk)
  {
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
  }

  std::vector<std::byte> bytes;
};

constexpr auto header_hex =
  "00000000"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "00000000"
  "00000000"
  "00000000";

constexpr auto legacy_tx_hex =
  "01000000"
  "01"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "ffffffff"
  "00"
  "ffffffff"
  "01"
  "0000000000000000"
  "00"
  "00000000";

} // namespace

TEST_CASE("block header parses and round-trips")
{
  auto const raw = hex_bytes(header_hex);
  REQUIRE(raw.size() == 80);

  auto const header = bitcoin::parse_block_header(std::span{raw});
  REQUIRE(header.has_value());
  CHECK(header->version() == 0);
  CHECK(header->prev_block_hash() == bitcoin::block_hash{});
  CHECK(header->merkle_root() == bitcoin::hash256{});
  CHECK(header->time().time_since_epoch() == std::chrono::seconds{0});
  CHECK(header->bits() == 0);
  CHECK(header->nonce() == 0);
  CHECK(bitcoin::serialized_size(*header) == 80);

  auto sink = vector_sink{};
  bitcoin::serialize(*header, sink);
  CHECK(sink.bytes == raw);
}

TEST_CASE("block header parsing rejects trailing bytes")
{
  auto raw = hex_bytes(header_hex);
  raw.push_back(byte(0x00));
  CHECK_FALSE(bitcoin::parse_block_header(std::span{raw}).has_value());
}

TEST_CASE("block parses and round-trips")
{
  auto raw = hex_bytes(header_hex);
  raw.push_back(byte(0x01));
  auto const tx = hex_bytes(legacy_tx_hex);
  raw.insert(raw.end(), tx.begin(), tx.end());

  auto const block = bitcoin::parse_block(std::span{raw});
  REQUIRE(block.has_value());

  auto const transactions = block->transactions();
  REQUIRE(transactions.size() == 1);
  CHECK(block->hash() == block->header().hash());
  CHECK(bitcoin::has_coinbase(*block));
  CHECK(bitcoin::serialized_size(*block) == raw.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*block, sink);
  CHECK(sink.bytes == raw);
}
