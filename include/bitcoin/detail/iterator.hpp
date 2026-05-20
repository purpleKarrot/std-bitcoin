// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/detail/arrow_proxy.hpp>

namespace bitcoin::detail {

template <typename T>
class iterator
{
public:
  using difference_type = std::ptrdiff_t;
  using value_type = decltype(std::declval<T>()[0]);
  using iterator_concept = std::random_access_iterator_tag;

  constexpr iterator() = default;
  iterator(T const& impl, std::size_t index)
    : _impl(&impl)
    , _index(index)
  {
  }

  [[nodiscard]] auto operator*() const -> value_type
  {
    assert(_impl != nullptr);
    return (*_impl)[_index];
  }

  [[nodiscard]] auto operator->() const
  {
    decltype(auto) ref = **this;
    if constexpr (std::is_reference_v<decltype(ref)>) {
      return std::addressof(ref);
    }
    else {
      return arrow_proxy{std::move(ref)};
    }
  }

  [[nodiscard]] auto operator[](difference_type n) const -> value_type
  {
    assert(_impl != nullptr);
    return (*_impl)[_index + n];
  }

  auto operator++() -> iterator&
  {
    ++_index;
    return *this;
  }

  auto operator++(int) -> iterator
  {
    iterator tmp = *this;
    ++_index;
    return tmp;
  }

  auto operator--() -> iterator&
  {
    --_index;
    return *this;
  }

  auto operator--(int) -> iterator
  {
    iterator tmp = *this;
    --_index;
    return tmp;
  }

  auto operator+=(difference_type n) -> iterator&
  {
    _index += n;
    return *this;
  }

  auto operator-=(difference_type n) -> iterator&
  {
    _index -= n;
    return *this;
  }

  friend auto operator+(iterator it, difference_type n) -> iterator
  {
    return it += n;
  }
  friend auto operator+(difference_type n, iterator it) -> iterator
  {
    return it += n;
  }

  friend auto operator-(iterator it, difference_type n) -> iterator
  {
    return it + (-n);
  }

  friend auto operator-(iterator lhs, iterator rhs) -> difference_type
  {
    assert(lhs._impl == rhs._impl);
    return lhs._index - rhs._index;
  }

  friend constexpr auto operator==(iterator lhs, iterator rhs) noexcept -> bool
  {
    assert(lhs._impl == rhs._impl);
    return lhs._index == rhs._index;
  }

  friend constexpr auto operator<=>(iterator lhs, iterator rhs) noexcept
  {
    assert(lhs._impl == rhs._impl);
    return lhs._index <=> rhs._index;
  }

private:
  T const* _impl = nullptr;
  std::size_t _index = 0;
};

template <typename T>
class random_access_view : public std::ranges::view_interface<T>
{
public:
  using iterator = detail::iterator<T>;
  using const_iterator = iterator;

  [[nodiscard]] auto begin() const noexcept -> iterator
  {
    return iterator(derived(), 0);
  }

  [[nodiscard]] auto end() const noexcept(noexcept(derived().size()))
    -> iterator
  {
    return iterator(derived(), derived().size());
  }

private:
  random_access_view() = default;

  [[nodiscard]] auto derived() const noexcept -> T const&
  {
    return static_cast<T const&>(*this);
  }

  friend T;
};

} // namespace bitcoin::detail
