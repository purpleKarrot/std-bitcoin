// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/coin.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <type_traits>
#include <unordered_map>

#include <doctest/doctest.h>

#include <bitcoin/coin_index.hpp>

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
    if (auto const it = coins.find(p); it != coins.end()) {
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
  auto const coin = bitcoin::coin{};

  CHECK(coin.value() == 0 * satoshi);
  CHECK(coin.output_script().empty());
  CHECK(coin.funding_height() == 0);
  CHECK_FALSE(is_coinbase(coin));
}

TEST_CASE("coin copies the output script and preserves its metadata")
{
  auto const bytes = std::array{byte(0x51), byte(0x21), byte(0x02)};
  auto script = bitcoin::script{std::span{bytes}};

  auto const coin = bitcoin::coin{42 * satoshi, script, 144, true};
  script.clear();

  CHECK(coin.value() == 42 * satoshi);
  CHECK(coin.funding_height() == 144);
  CHECK(is_coinbase(coin));
  CHECK(std::ranges::equal(as_bytes(coin.output_script()), std::span{bytes}));
}

TEST_CASE("coin equality compares all stored fields")
{
  auto const bytes = std::array{byte(0x51)};
  auto const script = bitcoin::script{std::span{bytes}};

  auto const a = bitcoin::coin{1 * satoshi, script, 7, false};
  auto const b = bitcoin::coin{1 * satoshi, script, 7, false};
  auto const different_height = bitcoin::coin{1 * satoshi, script, 8, false};
  auto const different_coinbase = bitcoin::coin{1 * satoshi, script, 7, true};

  CHECK(a == b);
  CHECK_FALSE(a == different_height);
  CHECK_FALSE(a == different_coinbase);
}

TEST_CASE("coin_index_ref forwards lookups to the wrapped index")
{
  auto const bytes = std::array{byte(0xac)};
  auto const script = bitcoin::script{std::span{bytes}};
  auto const prevout = bitcoin::outpoint{bitcoin::txid{}, 3};
  auto const expected = bitcoin::coin{5 * satoshi, script, 12};

  auto index = map_coin_index{};
  index.coins.emplace(prevout, expected);

  auto const ref = bitcoin::detail::coin_index_ref{index};

  auto const found = ref.lookup(prevout);
  REQUIRE(found.has_value());
  CHECK(*found == expected);
  CHECK(index.lookup_calls == 1);

  auto const missing = ref.lookup(bitcoin::outpoint{bitcoin::txid{}, 4});
  CHECK_FALSE(missing.has_value());
  CHECK(index.lookup_calls == 2);
}

TEST_CASE("coin_index_ref observes updates to the wrapped index")
{
  auto const updated_bytes = std::array{byte(0x52)};
  auto const prevout = bitcoin::outpoint{bitcoin::txid{}, 9};

  auto index = map_coin_index{};
  auto ref = bitcoin::detail::coin_index_ref{index};

  auto const updated_script = bitcoin::script{std::span{updated_bytes}};
  index.coins.insert_or_assign(
    prevout, bitcoin::coin{7 * satoshi, updated_script, 88, true});

  auto const found = ref.lookup(prevout);
  REQUIRE(found.has_value());
  CHECK(found->value() == 7 * satoshi);
  CHECK(found->funding_height() == 88);
  CHECK(is_coinbase(*found));
  CHECK(std::ranges::equal(as_bytes(found->output_script()),
                           std::span{updated_bytes}));
}
