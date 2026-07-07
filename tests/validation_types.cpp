// SPDX-License-Identifier: BSL-1.0

#include <format>

#include <doctest/doctest.h>

#include <bitcoin/validation_flags.hpp>
#include <bitcoin/validation_result.hpp>
#include <bitcoin/validation_status.hpp>

TEST_CASE("validation_flags string conversion")
{
  using flags = bitcoin::validation_flags;
  CHECK(std::format("{}", flags::none) == "NONE");
  CHECK(std::format("{}", flags::witness | flags::p2sh) == "P2SH|WITNESS");
}

TEST_CASE("validation_status reports success and formats")
{
  auto const ok = bitcoin::validation_status{};
  auto const not_ok = bitcoin::validation_status{1};

  CHECK(ok.ok());
  CHECK(static_cast<bool>(ok));
  CHECK_FALSE(not_ok.ok());
  CHECK_FALSE(static_cast<bool>(not_ok));

  CHECK(std::format("{}", ok) == "OK");
  CHECK(std::format("{}", not_ok) == "NOT OK");
}

TEST_CASE("validation_result stores fact and status")
{
  auto const success = bitcoin::validation_result<int>{42};
  auto const failure =
    bitcoin::validation_result<int>{bitcoin::validation_status{1}};

  CHECK(success.ok());
  CHECK(success.status().ok());
  CHECK(success.fact() == 42);

  CHECK_FALSE(failure.ok());
  CHECK_FALSE(failure.status().ok());
  CHECK(failure.fact() == 0);
}
