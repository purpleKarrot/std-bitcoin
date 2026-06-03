// SPDX-License-Identifier: BSL-1.0

#pragma once

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
consteval auto hex_decode(char const (&str)[N])
{
  static_assert((N - 1) % 2 == 0, "hex string must have even length");

  constexpr std::size_t out_size = (N - 1) / 2;

  auto out = std::array<std::byte, out_size>{};

  for (std::size_t i = 0; i < out_size; ++i) {
    auto hi = hex_value(str[i * 2]);
    auto lo = hex_value(str[i * 2 + 1]);
    out[i] = std::byte((hi << 4) | lo);
  }

  return out;
}

} // namespace detail

template <typename CharT, CharT... Cs>
consteval auto operator""_hex()
{
  constexpr char str[] = {Cs..., '\0'};
  return detail::hex_decode(str);
}

} // namespace hex_literal
