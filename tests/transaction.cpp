// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/transaction.hpp>

#include <array>
#include <cstdint>
#include <ranges>
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
  auto write(std::span<std::byte const> chunk) -> void
  {
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
  }

  std::vector<std::byte> bytes;
};

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

constexpr auto witness_tx_hex =
  "01000000"
  "0001"
  "01"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "ffffffff"
  "00"
  "ffffffff"
  "01"
  "0000000000000000"
  "00"
  "01"
  "02dead"
  "00000000";

constexpr auto witness_tx_alt_hex =
  "01000000"
  "0001"
  "01"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "ffffffff"
  "00"
  "ffffffff"
  "01"
  "0000000000000000"
  "00"
  "01"
  "02beef"
  "00000000";

} // namespace

TEST_CASE("transaction parsing rejects invalid input")
{
  auto const tx = bitcoin::parse_transaction({});
  CHECK(!tx.has_value());
}

TEST_CASE("default-constructed transaction is empty")
{
  auto const tx = bitcoin::transaction{};
  CHECK(tx.inputs().empty());
  CHECK(tx.outputs().empty());
}

TEST_CASE("legacy transaction parses and round-trips")
{
  auto const raw = hex_bytes(legacy_tx_hex);
  auto const tx = bitcoin::parse_transaction(std::span{raw});
  REQUIRE(tx.has_value());

  auto inputs = tx->inputs();
  auto outputs = tx->outputs();
  REQUIRE(inputs.size() == 1);
  REQUIRE(outputs.size() == 1);

  auto const input = inputs.front();
  auto const output = outputs.front();

  CHECK(input.previous_output().txid() == bitcoin::txid{});
  CHECK(input.previous_output().index() == 0xffffffffU);
  CHECK(input.script().empty());
  CHECK(input.sequence() == 0xffffffffU);
  CHECK(std::ranges::empty(input.witness()));

  CHECK(output.value() == 0 * bitcoin::units::satoshi);
  CHECK(output.script().empty());

  CHECK(std::ranges::equal(as_bytes(tx->id()), as_bytes(tx->witness_id())));
  CHECK(bitcoin::serialized_size(*tx) == raw.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*tx, sink);
  CHECK(sink.bytes == raw);
}

TEST_CASE("witness transaction exposes witness and has distinct identifiers")
{
  auto const raw = hex_bytes(witness_tx_hex);
  auto const tx = bitcoin::parse_transaction(std::span{raw});
  REQUIRE(tx.has_value());

  auto inputs = tx->inputs();
  REQUIRE(inputs.size() == 1);

  auto const input = inputs.front();
  auto witness = input.witness();
  REQUIRE(witness.size() == 1);

  auto const witness_item = witness.front();
  CHECK(std::ranges::equal(witness_item, hex_bytes("dead")));
  CHECK_FALSE(
    std::ranges::equal(as_bytes(tx->id()), as_bytes(tx->witness_id())));
  CHECK(bitcoin::serialized_size(*tx) == raw.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*tx, sink);
  CHECK(sink.bytes == raw);
}

TEST_CASE("transaction parsing rejects trailing bytes")
{
  auto raw = hex_bytes(legacy_tx_hex);
  raw.push_back(byte(0x00));
  auto const tx = bitcoin::parse_transaction(std::span{raw});
  CHECK(!tx.has_value());
}

TEST_CASE("tx_input equality includes witness")
{
  auto const raw1 = hex_bytes(witness_tx_hex);
  auto const raw2 = hex_bytes(witness_tx_alt_hex);
  auto const tx1 = bitcoin::parse_transaction(std::span{raw1});
  auto const tx2 = bitcoin::parse_transaction(std::span{raw2});
  REQUIRE(tx1.has_value());
  REQUIRE(tx2.has_value());
  CHECK_FALSE(tx1->inputs().front() == tx2->inputs().front());
}

TEST_CASE("outpoint hashing is consistent with equality")
{
  auto bytes = std::array<std::byte, 32>{};
  bytes[0] = byte(0x12);
  bytes[31] = byte(0x34);

  auto const txid = bitcoin::txid{std::span{bytes}};
  auto const a = bitcoin::outpoint{txid, 7};
  auto const b = bitcoin::outpoint{txid, 7};
  auto const c = bitcoin::outpoint{txid, 8};

  auto const hash = std::hash<bitcoin::outpoint>{};
  CHECK(a == b);
  CHECK(hash(a) == hash(b));
  CHECK(a != c);
}
