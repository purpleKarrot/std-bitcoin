// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/validation.hpp>

#include <doctest/doctest.h>

TEST_CASE("verification_flags string conversion")
{
  using bitcoin::verification_flags;
  CHECK(std::format("{}", verification_flags::none) == "NONE");

  auto const flags = verification_flags::witness | verification_flags::p2sh |
    verification_flags::dersig;
  CHECK(std::format("{}", flags) == "P2SH|DERSIG|WITNESS");
}
