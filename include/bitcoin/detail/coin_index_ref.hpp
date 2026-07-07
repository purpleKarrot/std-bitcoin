// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>

#include <bitcoin/coin_index.hpp>

namespace bitcoin::detail {

class coin_index_ref
{
public:
  template <coin_index T>
    requires(!std::same_as<std::remove_cvref_t<T>, coin_index_ref>)
  coin_index_ref(T& index) noexcept
    : _object(std::addressof(index))
    , _lookup([](void* object, outpoint const& p) {
      return static_cast<T*>(object)->lookup(p);
    })
  {
  }

  auto lookup(outpoint const& p) const { return _lookup(_object, p); }

private:
  using lookup_fn = std::optional<coin> (*)(void*, outpoint const&);

  void* _object;
  lookup_fn _lookup;
};

static_assert(coin_index<coin_index_ref>);

} // namespace bitcoin::detail
