// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace bitcoin::detail {

template <typename T>
  requires std::is_object_v<T>
struct arrow_proxy
{
  constexpr arrow_proxy(T const& value) noexcept(noexcept(T(value)))
    : _value(value)
  {
  }

  constexpr arrow_proxy(T&& value) noexcept(noexcept(T(std::move(value))))
    : _value(std::move(value))
  {
  }

  constexpr auto operator->() noexcept -> T* { return &_value; }
  constexpr auto operator->() const noexcept -> T const* { return &_value; }

private:
  T _value;
};

template <typename T>
arrow_proxy(T) -> arrow_proxy<T>;

} // namespace bitcoin::detail
