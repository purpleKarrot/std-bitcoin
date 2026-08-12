// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace hex_literal {
namespace detail {

consteval unsigned hex_value(char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + (c - 'a');
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + (c - 'A');
  }
  throw "invalid hex character";
}

template <std::size_t N>
struct hex_data
{
  static_assert((N - 1) % 2 == 0, "hex string must have even length");
  std::array<std::byte, (N - 1) / 2> bytes{};

  constexpr hex_data(char const (&str)[N]) // NOLINT
  {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      auto hi = hex_value(str[i * 2]);
      auto lo = hex_value(str[(i * 2) + 1]);
      bytes[i] = std::byte((hi << 4) | lo);
    }
  }
};

template <typename T, hex_data S>
consteval auto make_hash_id()
{
  auto result = S.bytes;
  static_assert(result.size() == sizeof(T));
  std::ranges::reverse(result);
  return T{result};
}

} // namespace detail

template <detail::hex_data S>
consteval auto operator""_hex()
{
  return S.bytes;
}

template <detail::hex_data S>
consteval auto operator""_block_hash()
{
  return detail::make_hash_id<bitcoin::block_hash, S>();
}

template <detail::hex_data S>
consteval auto operator""_txid()
{
  return detail::make_hash_id<bitcoin::txid, S>();
}

template <detail::hex_data S>
consteval auto operator""_wtxid()
{
  return detail::make_hash_id<bitcoin::wtxid, S>();
}

template <detail::hex_data S>
consteval auto operator""_hash256()
{
  return detail::make_hash_id<bitcoin::hash256, S>();
}

} // namespace hex_literal
