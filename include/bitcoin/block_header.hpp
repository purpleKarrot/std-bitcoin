// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <bitcoin/hash_id.hpp>
#include <bitcoin/serdes/byte_sink.hpp>

namespace bitcoin {

struct block_header
{
  std::int32_t version;
  bitcoin::block_hash prev_block_hash;
  bitcoin::hash256 merkle_root;
  std::chrono::sys_seconds time;
  std::uint32_t bits;
  std::uint32_t nonce;

  friend bool operator==(block_header const& lhs,
                         block_header const& rhs) noexcept = default;
  friend auto operator<=>(block_header const& lhs,
                          block_header const& rhs) noexcept = default;
};

[[nodiscard]] auto parse_block_header(std::span<std::byte const> raw)
  -> std::optional<block_header>;

namespace detail {

void serialize(block_header const& header, byte_sink_ref sink);

} // namespace detail

template <serdes::byte_sink Sink>
void serialize(block_header const& header, Sink& sink)
{
  detail::serialize(header, detail::byte_sink_ref{sink});
}

[[nodiscard]] auto serialized_size(block_header const& header) -> std::size_t;

} // namespace bitcoin
