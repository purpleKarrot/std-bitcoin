// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <bitcoin/hash_id.hpp>
#include <bitcoin/serialization/byte_sink.hpp>

namespace bitcoin {

class block_header;

[[nodiscard]] auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>;
void serialize(block_header const& header, serialization::byte_sink_ref sink);
[[nodiscard]] auto serialized_size(block_header const& header) -> std::size_t;

class block_header
{
public:
  constexpr block_header() noexcept = default;

  [[nodiscard]] auto hash() const noexcept -> bitcoin::block_hash;

  [[nodiscard]] constexpr auto version() const noexcept -> std::int32_t
  {
    return _version;
  }

  [[nodiscard]] constexpr auto prev_block_hash() const noexcept
    -> bitcoin::block_hash
  {
    return _prev_block_hash;
  }

  [[nodiscard]] constexpr auto merkle_root() const noexcept -> bitcoin::hash256
  {
    return _merkle_root;
  }

  [[nodiscard]] constexpr auto time() const noexcept -> std::chrono::sys_seconds
  {
    return std::chrono::sys_seconds{std::chrono::seconds{_time}};
  }

  [[nodiscard]] constexpr auto bits() const noexcept -> std::uint32_t
  {
    return _bits;
  }

  [[nodiscard]] constexpr auto nonce() const noexcept -> std::uint32_t
  {
    return _nonce;
  }

  friend auto operator==(block_header const& lhs,
                         block_header const& rhs) noexcept -> bool = default;

  friend void serialize(block_header const& header,
                        serialization::byte_sink_ref sink);
  friend auto serialized_size(block_header const& header) -> std::size_t;
  friend auto parse_block_header(std::span<std::byte const> raw)
    -> std::optional<block_header>;

private:
  std::int32_t _version{};
  bitcoin::block_hash _prev_block_hash;
  bitcoin::hash256 _merkle_root;
  std::uint32_t _time{};
  std::uint32_t _bits{};
  std::uint32_t _nonce{};
};

} // namespace bitcoin
