// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

#include <bitcoin/coin.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

template <typename T>
concept coin_index = requires(T const& m, outpoint p) {
  { m.lookup(p) } -> std::convertible_to<std::optional<coin>>;
};

namespace detail {

class coin_index_ref
{
public:
  template <coin_index T>
    requires(!std::same_as<std::remove_cvref_t<T>, coin_index_ref>)
  constexpr coin_index_ref(T& index) noexcept
    : _object(std::addressof(index))
    , _lookup([](void* object, outpoint const& p) {
      return static_cast<T*>(object)->lookup(p);
    })
  {
  }

  [[nodiscard]] auto lookup(outpoint const& p) const
  {
    return _lookup(_object, p);
  }

private:
  using lookup_fn = std::optional<coin> (*)(void*, outpoint const&);

  void* _object;
  lookup_fn _lookup;
};

static_assert(coin_index<coin_index_ref>);

} // namespace detail
} // namespace bitcoin
