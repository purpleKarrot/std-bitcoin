// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <format>

namespace bitcoin {

enum class validation_flags : std::uint_least32_t
{
  none = 0,
  p2sh = 1U << 0,                 // BIP16
  dersig = 1U << 2,               // BIP66
  nulldummy = 1U << 4,            // BIP147
  checklocktimeverify = 1U << 9,  // BIP65
  checksequenceverify = 1U << 10, // BIP112
  witness = 1U << 11,             // BIP141
  taproot = 1U << 17,             // BIPs 341 & 342
  all = p2sh
    | dersig
    | nulldummy
    | checklocktimeverify
    | checksequenceverify
    | witness
    | taproot,
};

[[nodiscard]] constexpr auto operator|(validation_flags l,
                                       validation_flags r) noexcept
{
  return validation_flags(static_cast<int>(l) | static_cast<int>(r));
}

} // namespace bitcoin

namespace std {

template <>
struct formatter<bitcoin::validation_flags>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    format_parse_context::iterator const it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(bitcoin::validation_flags flags, format_context& ctx) const
    -> format_context::iterator;
};

} // namespace std
