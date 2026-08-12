// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <format>

#include <doctest/doctest.h>

TEST_CASE("validation_flags string conversion")
{
  using flags = bitcoin::validation_flags;
  CHECK(std::format("{}", flags::none) == "NONE");
  CHECK(std::format("{}", flags::witness | flags::p2sh) == "P2SH|WITNESS");
}

TEST_CASE("validation_status reports success and formats")
{
  auto ok = bitcoin::validation_status{};
  auto not_ok = bitcoin::validation_status{"NOT OK"};

  CHECK(ok);
  CHECK_FALSE(not_ok);

  CHECK(std::format("{}", ok) == "OK");
  CHECK(std::format("{}", not_ok) == "NOT OK");
}
