// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/hash_id.hpp>

#include <array>
#include <cstddef>
#include <format>
#include <ranges>
#include <span>
#include <string_view>

namespace bitcoin::detail {

auto format_hash_id(std::span<std::byte const, 32> bytes,
                    std::formatter<std::string_view, char> const& formatter,
                    std::format_context& ctx) -> std::format_context::iterator
{
  static constexpr char const* hex = "0123456789abcdef";

  auto buffer = std::array<char, 64>{};
  auto* out = buffer.begin();

  for (auto const byte : std::views::reverse(bytes)) {
    auto const value = std::to_integer<unsigned>(byte);
    *out++ = hex[value >> 4];
    *out++ = hex[value & 0x0f];
  }

  return formatter.format(std::string_view{buffer.data(), buffer.size()}, ctx);
}

} // namespace bitcoin::detail
