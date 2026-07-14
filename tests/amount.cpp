// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/amount.hpp>

#include <doctest/doctest.h>

using bitcoin::amount;
using bitcoin::units::btc;
using bitcoin::units::satoshi;

TEST_CASE("amount construction")
{
  CHECK(amount{}.numerical_value_in(satoshi) == 0);
  CHECK(amount::zero().numerical_value_in(satoshi) == 0);
  CHECK((42 * satoshi).numerical_value_in(satoshi) == 42);
  CHECK(amount{1, btc}.numerical_value_in(satoshi) == 100'000'000);
}

TEST_CASE("amount arithmetic")
{
  CHECK((1 * satoshi) + (2 * satoshi) == 3 * satoshi);
  CHECK((5 * satoshi) - (2 * satoshi) == 3 * satoshi);
  CHECK((3 * satoshi) * 4 == 12 * satoshi);
  CHECK((10 * satoshi) / 3 == 3 * satoshi);

  auto total = 10 * satoshi;
  total += 5 * satoshi;
  total -= 3 * satoshi;
  CHECK(total == 12 * satoshi);
}

TEST_CASE("amount comparison")
{
  CHECK((1 * satoshi) < (2 * satoshi));
  CHECK((2 * satoshi) > (1 * satoshi));
  CHECK(((5 * satoshi) <=> (5 * satoshi)) == std::strong_ordering::equal);
}

TEST_CASE("amount formatting")
{
  CHECK(std::format("{}", 1 * satoshi) == "1 sat");
  CHECK(std::format("{}", amount{1, btc}) == "100000000 sat");
  CHECK(std::format("{}", 1 * btc) == "1 BTC");
  CHECK(std::format("{}", 1.5L * btc) == "1.5 BTC");
}

TEST_CASE("amount rejects boolean construction")
{
  CHECK_FALSE(std::is_constructible_v<amount, bool>);
}
