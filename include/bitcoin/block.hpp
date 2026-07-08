// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <bitcoin/block_header.hpp>
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
    : _header{header}
    , _transactions{std::move(transactions)}
  {
  }

  [[nodiscard]] auto header() const noexcept -> block_header const&
  {
    return _header;
  }

  [[nodiscard]] auto transactions() const -> transaction_view
  {
    return _transactions;
  }

  friend bool operator==(block const& lhs, block const& rhs) noexcept = default;
  friend void detail::serialize(block const& b, detail::byte_sink_ref sink);
  friend auto serialized_size(block const& b) -> std::size_t;

private:
  block_header _header;
  std::vector<transaction> _transactions;
  friend struct _impl_access;
};

[[nodiscard]] auto parse_block(std::span<std::byte const> raw)
  -> std::optional<block>;

template <serdes::byte_sink Sink>
void serialize(block const& b, Sink& sink)
{
  detail::serialize(b, detail::byte_sink_ref{sink});
}

[[nodiscard]] auto serialized_size(block const& b) -> std::size_t;

} // namespace bitcoin
