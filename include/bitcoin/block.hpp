// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <copy_on_write.hpp>

#include <bitcoin/block_header.hpp>
#include <bitcoin/hash_id.hpp>
#include <bitcoin/serdes/byte_sink.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

class block;

namespace detail {

void serialize(block const& b, byte_sink_ref sink);

} // namespace detail

class block
{
public:
  using transaction_view = std::span<transaction const>;

  block() = default;
  explicit block(block_header header, std::vector<transaction> transactions)
    : _impl{std::in_place, header, std::move(transactions)}
  {
  }

  [[nodiscard]] auto header() const -> block_header const&
  {
    return _impl->header;
  }

  [[nodiscard]] auto transactions() const -> transaction_view
  {
    return _impl->transactions;
  }

  friend bool operator==(block const& lhs, block const& rhs) = default;
  friend void detail::serialize(block const& b, detail::byte_sink_ref sink);
  friend auto serialized_size(block const& b) -> std::size_t;

private:
  struct implementation
  {
    block_header header;
    std::vector<transaction> transactions;

    friend bool operator==(implementation const&,
                           implementation const&) = default;
  };

  xyz::copy_on_write<implementation> _impl;
};

[[nodiscard]] auto parse_block(std::span<std::byte const> raw)
  -> std::optional<block>;

template <serdes::byte_sink Sink>
void serialize(block const& b, Sink& sink)
{
  detail::serialize(b, detail::byte_sink_ref{sink});
}

[[nodiscard]] auto serialized_size(block const& b) -> std::size_t;

inline auto detail::block_hash_policy::operator()(block const& b) const
  -> std::array<std::byte, 32>
{
  return operator()(b.header());
}

} // namespace bitcoin
