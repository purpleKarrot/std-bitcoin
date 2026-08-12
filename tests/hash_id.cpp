// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <type_traits>

#include <doctest/doctest.h>

#include "hex.hpp"

namespace {

constexpr auto sample_bytes() -> std::array<std::byte, 32>
{
  auto bytes = std::array<std::byte, 32>{};
  bytes[0] = std::byte{0x01};
  bytes[1] = std::byte{0x23};
  bytes[30] = std::byte{0x45};
  bytes[31] = std::byte{0x67};
  return bytes;
}

} // namespace

static_assert(!static_cast<bool>(bitcoin::txid{}));
static_assert(!std::is_same_v<bitcoin::txid, bitcoin::wtxid>);
static_assert(!std::is_same_v<bitcoin::txid, bitcoin::block_hash>);
static_assert(!std::is_same_v<bitcoin::txid, bitcoin::hash256>);

TEST_CASE("hash ids default to all zero bytes")
{
  auto id = bitcoin::txid{};
  auto bytes = as_bytes(id);

  CHECK(bytes.size() == 32);
  CHECK(!static_cast<bool>(id));

  for (auto byte : bytes) {
    CHECK(byte == std::byte{0x00});
  }
}

TEST_CASE("hash ids copy and expose wire-order bytes")
{
  auto bytes = sample_bytes();
  auto id = bitcoin::txid{std::span{bytes}};

  CHECK(static_cast<bool>(id));
  CHECK(std::ranges::equal(as_bytes(id), std::span{bytes}));
}

TEST_CASE("hash ids compare by stored bytes")
{
  auto bytes = sample_bytes();
  auto a = bitcoin::txid{std::span{bytes}};
  auto b = bitcoin::txid{std::span{bytes}};

  bytes[0] = std::byte{0x02};
  auto c = bitcoin::txid{std::span{bytes}};

  CHECK(a == b);
  CHECK(a != c);
  CHECK((a < c));
}

TEST_CASE("hash ids format as lowercase hex in display byte order")
{
  using namespace hex_literal;
  auto bytes = sample_bytes();
  auto id = bitcoin::txid{std::span{bytes}};

  CHECK(
    id
    == "6745000000000000000000000000000000000000000000000000000000002301"_txid);
  CHECK(std::format("{}", id)
        == "6745000000000000000000000000000000000000000000000000000000002301");
}

TEST_CASE("hash support is consistent with equality")
{
  auto bytes = sample_bytes();
  auto a = bitcoin::txid{std::span{bytes}};
  auto b = bitcoin::txid{std::span{bytes}};

  bytes[31] = std::byte{0x66};
  auto c = bitcoin::txid{std::span{bytes}};

  auto hash = std::hash<bitcoin::txid>{};
  CHECK(hash(a) == hash(b));
  CHECK(a == b);
  CHECK(a != c);
}
