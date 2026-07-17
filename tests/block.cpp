// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <array>
#include <chrono>
#include <span>
#include <vector>

#include <doctest/doctest.h>

#include "hex.hpp"

using namespace hex_literal;

namespace {

struct vector_sink
{
  void write(std::span<std::byte const> chunk)
  {
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
  }

  std::vector<std::byte> bytes;
};

constexpr auto header_buf =
  "00000000"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "0000000000000000000000000000000000000000000000000000000000000000"
  "00000000"
  "00000000"
  "00000000"_hex;

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

constexpr auto string_representation = std::string_view{
  "CBlock("
  "hash=14508459b221041eab257d2baaa7459775ba748246c8403609eb708f0e57e74b, "
  "ver=0x00000000, "
  "hashPrevBlock="
  "0000000000000000000000000000000000000000000000000000000000000000, "
  "hashMerkleRoot="
  "0000000000000000000000000000000000000000000000000000000000000000, nTime=0, "
  "nBits=00000000, nNonce=0, vtx=1)\n"
  "  CTransaction(hash=2fb7d2ab4e, ver=1, vin.size=1, vout.size=1, "
  "nLockTime=0)\n"
  "    CTxIn(COutPoint(0000000000, 4294967295), coinbase )\n"
  "    CScriptWitness()\n"
  "    CTxOut(nValue=0.00000000, scriptPubKey=)\n\n"};

} // namespace

TEST_CASE("block header parses and round-trips")
{
  REQUIRE(header_buf.size() == 80);

  auto const header = bitcoin::parse_block_header(header_buf);
  REQUIRE(header.has_value());
  CHECK(header->version == 0);
  CHECK(header->prev_block_hash == bitcoin::block_hash{});
  CHECK(header->merkle_root == bitcoin::hash256{});
  CHECK(header->time.time_since_epoch() == std::chrono::seconds{0});
  CHECK(header->bits == 0);
  CHECK(header->nonce == 0);
  CHECK(bitcoin::serialized_size(*header) == 80);

  auto sink = vector_sink{};
  bitcoin::serialize(*header, sink);
  CHECK(std::ranges::equal(sink.bytes, header_buf));
}

TEST_CASE("block header parsing rejects trailing bytes")
{
  auto raw = std::vector(header_buf.begin(), header_buf.end());
  raw.push_back(std::byte{0x00});
  CHECK_FALSE(bitcoin::parse_block_header(std::span{raw}).has_value());
}

TEST_CASE("block parses and round-trips")
{
  auto raw = std::vector(header_buf.begin(), header_buf.end());
  raw.push_back(std::byte{0x01});
  raw.insert(raw.end(), legacy_tx.begin(), legacy_tx.end());

  auto const block = bitcoin::parse_block(std::span{raw});
  REQUIRE(block.has_value());

  auto const transactions = block->transactions();
  REQUIRE(transactions.size() == 1);
  CHECK(bitcoin::block_hash{*block} == bitcoin::block_hash{block->header()});
  CHECK(bitcoin::has_coinbase(*block));
  CHECK(bitcoin::serialized_size(*block) == raw.size());

  auto sink = vector_sink{};
  bitcoin::serialize(*block, sink);
  CHECK(sink.bytes == raw);

  CHECK(std::format("{}", *block) == string_representation);
}
