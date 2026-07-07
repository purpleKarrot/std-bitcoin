// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/validation_flags.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace {

constexpr bool is_set(bitcoin::validation_flags f,
                      bitcoin::validation_flags flag) noexcept
{
  return (static_cast<int>(f) & static_cast<int>(flag)) != 0;
}

} // namespace

auto std::formatter<bitcoin::validation_flags>::format(
  bitcoin::validation_flags flags, std::format_context& ctx) const
  -> std::format_context::iterator
{
  using flag_t = bitcoin::validation_flags;
  using namespace std::string_view_literals;

  static constexpr auto entries = std::array{
    std::pair{flag_t::p2sh, "P2SH"sv},
    std::pair{flag_t::dersig, "DERSIG"sv},
    std::pair{flag_t::nulldummy, "NULLDUMMY"sv},
    std::pair{flag_t::checklocktimeverify, "CHECKLOCKTIMEVERIFY"sv},
    std::pair{flag_t::checksequenceverify, "CHECKSEQUENCEVERIFY"sv},
    std::pair{flag_t::witness, "WITNESS"sv},
    std::pair{flag_t::taproot, "TAPROOT"sv},
  };

  auto out = ctx.out();

  if (flags == flag_t::none) {
    return std::ranges::copy("NONE"sv, out).out;
  }

  bool first = true;
  for (auto const& [f, name] : entries) {
    if (!is_set(flags, f)) {
      continue;
    }

    if (!first) {
      *out++ = '|';
    }

    out = std::ranges::copy(name, out).out;
    first = false;
  }

  return out;
}
