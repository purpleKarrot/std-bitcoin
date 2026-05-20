// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/block_header.hpp>
#include <bitcoin/detail/chain_erasure.hpp>
#include <bitcoin/detail/iterator.hpp>

namespace bitcoin {

template <typename T>
concept chain =
  std::ranges::view<T> && std::ranges::random_access_range<T const> &&
  std::ranges::sized_range<T const> &&
  std::convertible_to<std::ranges::range_reference_t<T const>, block_header>;

class any_chain : public detail::random_access_view<any_chain>
{
public:
  any_chain() noexcept = default;

  template <typename T>
    requires(chain<std::remove_cvref_t<T>> &&
             !std::same_as<std::remove_cvref_t<T>, any_chain>)
  any_chain(T&& object)
    : _impl(std::forward<T>(object))
  {
  }

  [[nodiscard]] auto operator[](std::size_t index) const -> block_header
  {
    return _impl[index];
  }

  [[nodiscard]] auto size() const -> std::size_t;
  [[nodiscard]] auto height() const -> std::size_t;

  [[nodiscard]] auto mismatch(any_chain const& other) const
    -> std::ranges::mismatch_result<iterator, iterator>;

  [[nodiscard]] auto starts_with(any_chain const& prefix) const -> bool;

private:
  detail::chain_erasure _impl;
};

} // namespace bitcoin
