// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <array>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>
#include <mp-units/framework.h>

#include "hex.hpp"

using namespace hex_literal;

namespace {

struct vector_sink
{
  auto write(std::span<std::byte const> chunk) -> void
  {
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
  }

  std::vector<std::byte> bytes;
};

constexpr auto legacy_tx =
  "01000000"
  "01"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "ffffffff"
  "00"
  "ffffffff"
  "01"
  "0000000000000000"
  "00"
  "00000000"_hex;

constexpr auto witness_tx =
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
  "00000000"_hex;

constexpr auto witness_tx_alt =
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
  "00000000"_hex;

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
  auto const tx = bitcoin::parse_transaction(legacy_tx);
  REQUIRE(tx.has_value());

  auto inputs = tx->inputs();
  auto outputs = tx->outputs();
  REQUIRE(inputs.size() == 1);
  REQUIRE(outputs.size() == 1);

  auto const input = inputs.front();
  auto const output = outputs.front();

  CHECK(input.prevout().txid() == bitcoin::txid{});
  CHECK(input.prevout().index() == 0xffffffffU);
  CHECK(input.script().empty());
  CHECK(input.sequence() == 0xffffffffU);
  CHECK(std::ranges::empty(input.witness()));

  CHECK(output.value() == 0 * bitcoin::units::satoshi);
  CHECK(output.script().empty());

  CHECK(std::ranges::equal(as_bytes(bitcoin::txid{*tx}),
                           as_bytes(bitcoin::wtxid{*tx})));
  CHECK(bitcoin::serialized_size(*tx) == legacy_tx.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*tx, sink);
  CHECK(std::ranges::equal(sink.bytes, legacy_tx));
}

TEST_CASE("witness transaction exposes witness and has distinct identifiers")
{
  auto const tx = bitcoin::parse_transaction(witness_tx);
  REQUIRE(tx.has_value());

  auto inputs = tx->inputs();
  REQUIRE(inputs.size() == 1);

  auto const input = inputs.front();
  auto witness = input.witness();
  REQUIRE(witness.size() == 1);

  auto const witness_item = witness.front();
  CHECK(std::ranges::equal(witness_item, "dead"_hex));
  CHECK_FALSE(std::ranges::equal(as_bytes(bitcoin::txid{*tx}),
                                 as_bytes(bitcoin::wtxid{*tx})));
  CHECK(bitcoin::serialized_size(*tx) == witness_tx.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*tx, sink);
  CHECK(std::ranges::equal(sink.bytes, witness_tx));
}

TEST_CASE("transaction parsing rejects trailing bytes")
{
  auto raw = std::vector(legacy_tx.begin(), legacy_tx.end());
  raw.push_back(std::byte{0x00});
  auto const tx = bitcoin::parse_transaction(std::span{raw});
  CHECK(!tx.has_value());
}

TEST_CASE("tx_input equality includes witness")
{
  auto const tx1 = bitcoin::parse_transaction(witness_tx);
  auto const tx2 = bitcoin::parse_transaction(witness_tx_alt);
  REQUIRE(tx1.has_value());
  REQUIRE(tx2.has_value());
  CHECK_FALSE(tx1->inputs().front() == tx2->inputs().front());
}

TEST_CASE("outpoint hashing is consistent with equality")
{
  auto bytes = std::array<std::byte, 32>{};
  bytes[0] = std::byte{0x12};
  bytes[31] = std::byte{0x34};

  auto const txid = bitcoin::txid{std::span{bytes}};
  auto const a = bitcoin::outpoint{txid, 7};
  auto const b = bitcoin::outpoint{txid, 7};
  auto const c = bitcoin::outpoint{txid, 8};

  auto const hash = std::hash<bitcoin::outpoint>{};
  CHECK(a == b);
  CHECK(hash(a) == hash(b));
  CHECK(a != c);
}
