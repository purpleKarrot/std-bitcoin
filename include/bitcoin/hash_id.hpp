// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>

namespace bitcoin {

class block;
struct block_header;
class transaction;

namespace detail {

struct block_hash_policy
{
  void operator()(block const& b, std::span<std::byte, 32> dst) const;
  void operator()(block_header const& hdr, std::span<std::byte, 32> dst) const;
};

struct txid_policy
{
  void operator()(transaction const& tx, std::span<std::byte, 32> dst) const;
};

struct wtxid_policy
{
  void operator()(transaction const& tx, std::span<std::byte, 32> dst) const;
};

struct hash256_policy
{
};

template <class T, class Policy>
concept is_hash_source = requires(T const& src, std::span<std::byte, 32> dst) {
  { Policy{}(src, dst) };
};

template <class Policy>
class basic_hash_id
{
public:
  constexpr basic_hash_id() noexcept = default;

  constexpr explicit basic_hash_id(
    std::span<std::byte const, 32> bytes) noexcept
  {
    std::ranges::copy(bytes, _value.begin());
  }

  template <is_hash_source<Policy> T>
  explicit basic_hash_id(T const& src)
  {
    Policy{}(src, _value);
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
  {
    return *this != basic_hash_id{};
  }

private:
  friend constexpr auto as_bytes(basic_hash_id const& hash) noexcept
  {
    return std::span{hash._value};
  }

  friend constexpr bool operator==(basic_hash_id const&,
                                   basic_hash_id const&) noexcept = default;
  friend constexpr auto operator<=>(basic_hash_id const&,
                                    basic_hash_id const&) noexcept = default;

  std::array<std::byte, 32> _value{};
};

auto format_hash_id(std::span<std::byte const, 32> bytes,
                    std::formatter<std::string_view, char> const& formatter,
                    std::format_context& ctx) -> std::format_context::iterator;

} // namespace detail

using block_hash = detail::basic_hash_id<detail::block_hash_policy>;
using txid = detail::basic_hash_id<detail::txid_policy>;
using wtxid = detail::basic_hash_id<detail::wtxid_policy>;
using hash256 = detail::basic_hash_id<detail::hash256_policy>;

} // namespace bitcoin

template <class Tag>
struct std::formatter<bitcoin::detail::basic_hash_id<Tag>, char>
{
  constexpr auto parse(std::format_parse_context& ctx)
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
  [[nodiscard]] auto operator()(
    bitcoin::detail::basic_hash_id<Tag> const& hash) const noexcept
  {
    auto const bytes = as_bytes(hash);
    auto const* data = reinterpret_cast<char const*>(bytes.data());
    return std::hash<std::string_view>{}(std::string_view{data, bytes.size()});
  }
};
