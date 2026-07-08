// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <beman/any_view/any_view.hpp>

#include <bitcoin/block_header.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

class block;

namespace detail {

struct block_data;
void serialize(block const& b, byte_sink_ref sink);

} // namespace detail

class block
{
  static constexpr auto sized_random_access =
    beman::any_view::any_view_options::random_access
    | beman::any_view::any_view_options::sized;

public:
  using transaction_view =
    beman::any_view::any_view<transaction, sized_random_access, transaction>;

  block();
  explicit block(detail::block_data data);

  [[nodiscard]] auto header() const noexcept -> block_header const&;
  [[nodiscard]] auto transactions() const -> transaction_view;

  friend bool operator==(block const& lhs, block const& rhs) noexcept;
  friend void detail::serialize(block const& b, detail::byte_sink_ref sink);
  friend auto serialized_size(block const& b) -> std::size_t;

private:
  block_header _header;
  std::shared_ptr<detail::block_data const> _data;
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
