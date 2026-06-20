// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace bitcoin::detail {

template <typename T>
  requires std::is_object_v<T>
struct arrow_proxy
{
  constexpr auto operator->() noexcept -> T* { return &_value; }
  T _value;
};

} // namespace bitcoin::detail
