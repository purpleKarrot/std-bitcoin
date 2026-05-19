// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>

namespace bitcoin::detail {

template <class Tag>
class basic_hash_id
{
public:
  constexpr basic_hash_id() noexcept = default;

  constexpr explicit basic_hash_id(
    std::span<std::byte const, 32> bytes) noexcept
  {
    std::ranges::copy(bytes, _value.begin());
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
  {
    return *this != basic_hash_id{};
  }

private:
  friend constexpr auto as_bytes(basic_hash_id const& hash) noexcept
    -> std::span<std::byte const, 32>
  {
    return {hash._value};
  }

  friend constexpr auto operator==(basic_hash_id const&,
                                   basic_hash_id const&) noexcept
    -> bool = default;
  friend constexpr auto operator<=>(basic_hash_id const&,
                                    basic_hash_id const&) noexcept = default;

  std::array<std::byte, 32> _value{};
};

auto format_hash_id(std::span<std::byte const, 32> bytes,
                    std::formatter<std::string_view, char> const& formatter,
                    std::format_context& ctx) -> std::format_context::iterator;

} // namespace bitcoin::detail

template <class Tag>
struct std::formatter<bitcoin::detail::basic_hash_id<Tag>, char>
{
  constexpr auto parse(std::format_parse_context& ctx)
    -> std::format_parse_context::iterator
  {
    return _formatter.parse(ctx);
  }

  auto format(bitcoin::detail::basic_hash_id<Tag> const& hash,
              std::format_context& ctx) const
  {
    return bitcoin::detail::format_hash_id(as_bytes(hash), _formatter, ctx);
  }

private:
  std::formatter<std::string_view, char> _formatter;
};

template <class Tag>
struct std::hash<bitcoin::detail::basic_hash_id<Tag>>
{
  [[nodiscard]] auto operator()(bitcoin::detail::basic_hash_id<Tag> const& hash)
    const noexcept -> std::size_t
  {
    auto const bytes = as_bytes(hash);
    auto const* data = reinterpret_cast<char const*>(bytes.data());
    return std::hash<std::string_view>{}(std::string_view{data, bytes.size()});
  }
};
