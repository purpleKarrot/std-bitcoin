// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/utxo.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <doctest/doctest.h>

#include <bitcoin/amount.hpp>
#include <bitcoin/coin.hpp>
#include <bitcoin/detail/iterator.hpp>
#include <bitcoin/script.hpp>

namespace {

using coin_row = std::span<bitcoin::coin const>;
using spent_coin_rows = std::span<coin_row const>;

auto make_coin(std::int64_t value, std::size_t height, bool coinbase = false)
  -> bitcoin::coin
{
  return {value * bitcoin::units::satoshi, bitcoin::script{}, height, coinbase};
}

} // namespace

static_assert(bitcoin::coin_view<coin_row>);
static_assert(bitcoin::spent_coins<spent_coin_rows>);
static_assert(bitcoin::coin_view<bitcoin::tx_spent_coins>);
static_assert(bitcoin::spent_coins<bitcoin::any_spent_coins>);

TEST_CASE("any_spent_coins default construction")
{
  CHECK(bitcoin::any_spent_coins{}.empty());
}

TEST_CASE("any_spent_coins exposes transaction rows")
{
  auto const first = std::array{
    make_coin(1, 100),
    make_coin(2, 101, true),
  };
  auto const second = std::array{
    make_coin(3, 102),
  };
  auto const rows = std::array{
    coin_row(first.data(), first.size()),
    coin_row(second.data(), second.size()),
  };

  auto const coins =
    bitcoin::any_spent_coins(spent_coin_rows(rows.data(), rows.size()));

  CHECK(coins.size() == 2);
  CHECK(coins[0].size() == 2);
  CHECK(coins[1].size() == 1);
  CHECK(coins[0][0].value() == 1 * bitcoin::units::satoshi);
  CHECK(coins[0][1].value() == 2 * bitcoin::units::satoshi);
  CHECK(coins[0][1].funding_height() == 101);
  CHECK(is_coinbase(coins[0][1]));
}

TEST_CASE("tx_spent_coins keeps its own erased handle copy")
{
  auto const first = std::array{
    make_coin(5, 200),
  };
  auto const second = std::array{
    make_coin(8, 201),
    make_coin(13, 202),
  };
  auto const rows = std::array{
    coin_row(first.data(), first.size()),
    coin_row(second.data(), second.size()),
  };

  auto const row =
    bitcoin::any_spent_coins(spent_coin_rows(rows.data(), rows.size()))[1];

  CHECK(row.size() == 2);
  CHECK(row[0].value() == 8 * bitcoin::units::satoshi);
  CHECK(row[1].funding_height() == 202);
}

TEST_CASE("any_spent_coins iterators support random access")
{
  auto const first = std::array{
    make_coin(21, 300),
  };
  auto const second = std::array{
    make_coin(34, 301),
  };
  auto const rows = std::array{
    coin_row(first.data(), first.size()),
    coin_row(second.data(), second.size()),
  };

  auto const coins =
    bitcoin::any_spent_coins(spent_coin_rows(rows.data(), rows.size()));
  auto const it = coins.begin();

  CHECK(it[0].size() == 1);
  CHECK(it[1][0].value() == 34 * bitcoin::units::satoshi);
  CHECK((coins.end() - coins.begin()) == 2);
}
