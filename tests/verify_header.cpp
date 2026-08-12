// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <chrono>

#include <doctest/doctest.h>

#include "hex.hpp"

using namespace std::chrono_literals;
using namespace hex_literal;

TEST_CASE("header intrinsic valid")
{
  // The genesis header has valid proof of work.
  auto header = bitcoin::block_header{
    .version = 1,
    .prev_block_hash = bitcoin::block_hash{},
    .merkle_root =
      "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"_hash256,
    .time = std::chrono::sys_seconds{1231006505s},
    .bits = 0x1d00ffff,
    .nonce = 2083236893,
  };

  CHECK(bitcoin::verify(header));
  CHECK(std::format("{}", bitcoin::verify(header)) == "OK");
}

TEST_CASE("header intrinsic high hash")
{
  // Decrementing the nonce invalidates the proof of work.
  auto header = bitcoin::block_header{
    .version = 1,
    .prev_block_hash = bitcoin::block_hash{},
    .merkle_root =
      "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"_hash256,
    .time = std::chrono::sys_seconds{1231006505s},
    .bits = 0x1d00ffff,
    .nonce = 2083236892,
  };

  CHECK(!bitcoin::verify(header));
  CHECK(std::format("{}", bitcoin::verify(header)) == "high-hash");
}
