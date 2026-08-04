// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include <doctest/doctest.h>
#include <mp-units/framework.h>

namespace {

using namespace bitcoin::units;

constexpr auto byte(unsigned value) noexcept -> std::byte
{
  return std::byte{static_cast<std::uint8_t>(value)};
}

struct optional_coin_index
{
  auto lookup(bitcoin::outpoint const&) const -> std::optional<bitcoin::coin>
  {
    return std::nullopt;
  }
};

struct coin_returning_index
{
  bitcoin::coin value;

  auto lookup(bitcoin::outpoint const&) const -> bitcoin::coin { return value; }
};

struct mutable_only_coin_index
{
  auto lookup(bitcoin::outpoint const&) -> std::optional<bitcoin::coin>
  {
    return std::nullopt;
  }
};

struct invalid_coin_index
{
  auto lookup(bitcoin::outpoint const&) const -> int { return 0; }
};

struct map_coin_index
{
  auto lookup(bitcoin::outpoint const& p) const -> std::optional<bitcoin::coin>
  {
    ++lookup_calls;
    if (auto it = coins.find(p); it != coins.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  std::unordered_map<bitcoin::outpoint, bitcoin::coin> coins;
  mutable std::size_t lookup_calls = 0;
};

} // namespace

static_assert(bitcoin::coin_index<optional_coin_index>);
static_assert(bitcoin::coin_index<coin_returning_index>);
static_assert(!bitcoin::coin_index<mutable_only_coin_index>);
static_assert(!bitcoin::coin_index<invalid_coin_index>);

TEST_CASE("coin defaults to a zero-valued non-coinbase output")
{
  auto coin = bitcoin::coin{};

  CHECK(bitcoin::value(coin) == 0 * satoshi);
  CHECK(bitcoin::output_script(coin).empty());
  CHECK(bitcoin::funding_height(coin) == 0);
  CHECK_FALSE(bitcoin::is_coinbase(coin));
}

TEST_CASE("coin copies the output script and preserves its metadata")
{
  auto bytes = std::array{byte(0x51), byte(0x21), byte(0x02)};
  auto script = bitcoin::script{std::span{bytes}};

  auto coin = bitcoin::coin{42 * satoshi, script, 144, true};
  script.clear();

  CHECK(bitcoin::value(coin) == 42 * satoshi);
  CHECK(bitcoin::funding_height(coin) == 144);
  CHECK(bitcoin::is_coinbase(coin));
  CHECK(std::ranges::equal(as_bytes(bitcoin::output_script(coin)),
                           std::span{bytes}));
}

TEST_CASE("coin equality compares all stored fields")
{
  auto bytes = std::array{byte(0x51)};
  auto script = bitcoin::script{std::span{bytes}};

  auto a = bitcoin::coin{1 * satoshi, script, 7, false};
  auto b = bitcoin::coin{1 * satoshi, script, 7, false};
  auto different_height = bitcoin::coin{1 * satoshi, script, 8, false};
  auto different_coinbase = bitcoin::coin{1 * satoshi, script, 7, true};

  CHECK(a == b);
  CHECK_FALSE(a == different_height);
  CHECK_FALSE(a == different_coinbase);
}
