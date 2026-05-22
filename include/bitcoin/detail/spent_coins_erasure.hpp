// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/detail/erasure.hpp>

namespace bitcoin {
class coin;
}

namespace bitcoin::detail {

struct spent_coins_erasure_vtable
{
  void (*copy)(void const* src, void* dst);
  void (*move)(void* src, void* dst) noexcept;
  void (*destroy)(void* storage) noexcept;
  std::size_t (*size)(void const* storage);
  std::size_t (*inputs_size)(void const* storage, std::size_t tx_index);
  coin (*get_coin)(void const* storage, std::size_t tx_index,
                   std::size_t input_index);
};

struct spent_coins_erasure_policy
{
  using vtable = spent_coins_erasure_vtable;

  static inline auto const empty_vtable = vtable{
    .copy = [](void const*, void*) {},
    .move = [](void*, void*) noexcept {},
    .destroy = [](void*) noexcept {},
    .size = [](void const*) -> std::size_t { return 0; },
    .inputs_size = [](void const*, std::size_t) -> std::size_t { return 0; },
    .get_coin = [](void const*, std::size_t, std::size_t) -> coin {
      assert(false);
      return {};
    },
  };
};

class spent_coins_erasure : private erasure<spent_coins_erasure_policy>
{
  using base = erasure<spent_coins_erasure_policy>;

public:
  spent_coins_erasure() noexcept = default;

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, spent_coins_erasure>)
  spent_coins_erasure(T&& value)
  {
    if (std::ranges::empty(value)) {
      return;
    }

    using value_type = std::remove_cvref_t<T>;
    this->template construct<value_type>(std::forward<T>(value),
                                         vtable_for<value_type>);
  }

  [[nodiscard]] auto size() const -> std::size_t
  {
    return this->table().size(this->storage());
  }

  [[nodiscard]] auto inputs_size(std::size_t tx_index) const -> std::size_t
  {
    return this->table().inputs_size(this->storage(), tx_index);
  }

  [[nodiscard]] auto get_coin(std::size_t tx_index,
                              std::size_t input_index) const -> coin
  {
    return this->table().get_coin(this->storage(), tx_index, input_index);
  }

private:
  template <typename T>
  static inline auto const vtable_for = vtable{
    .copy =
      [](void const* src, void* dst) {
        std::construct_at(raw_storage<T>(dst), *storage<T>(src));
      },
    .move =
      [](void* src, void* dst) noexcept {
        std::construct_at(raw_storage<T>(dst), std::move(*storage<T>(src)));
        std::destroy_at(storage<T>(src));
      },
    .destroy = [](void* p) noexcept { std::destroy_at(storage<T>(p)); },
    .size = [](void const* p) -> std::size_t {
      return std::ranges::size(object<T>(p));
    },
    .inputs_size = [](void const* p, std::size_t tx_index) -> std::size_t {
      return std::ranges::size(object<T>(p)[tx_index]);
    },
    .get_coin = [](void const* p, std::size_t tx_index, std::size_t input_index)
      -> coin { return object<T>(p)[tx_index][input_index]; },
  };
};

} // namespace bitcoin::detail
