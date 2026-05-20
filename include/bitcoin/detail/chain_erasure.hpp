// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/block_header.hpp>
#include <bitcoin/detail/erasure.hpp>

namespace bitcoin::detail {

struct chain_erasure_vtable
{
  void (*copy)(void const* src, void* dst);
  void (*move)(void* src, void* dst) noexcept;
  void (*destroy)(void* storage) noexcept;
  std::size_t (*size)(void const* storage);
  block_header (*get)(void const* storage, std::size_t index);
  bool (*starts_with)(void const* storage, void const* other);
  std::size_t (*mismatch)(void const* storage, void const* other);
};

struct chain_erasure_policy
{
  using vtable = chain_erasure_vtable;

  static inline auto const empty_vtable = vtable{
    .copy = [](void const*, void*) {},
    .move = [](void*, void*) noexcept {},
    .destroy = [](void*) noexcept {},
    .size = [](void const*) -> std::size_t { return 0; },
    .get = [](void const*, std::size_t) -> block_header {
      assert(false);
      return {};
    },
  };
};

class chain_erasure : private erasure<chain_erasure_policy>
{
  using base = erasure<chain_erasure_policy>;

public:
  chain_erasure() noexcept = default;

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, chain_erasure>)
  chain_erasure(T&& value)
  {
    auto const& const_value = value;
    if (std::ranges::empty(const_value)) {
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

  [[nodiscard]] auto operator[](std::size_t index) const -> block_header
  {
    return this->table().get(this->storage(), index);
  }

  [[nodiscard]] auto same_type(chain_erasure const& other) const noexcept
    -> bool
  {
    return base::same_type(other);
  }

  [[nodiscard]] auto has_starts_with() const noexcept -> bool
  {
    return this->table().starts_with != nullptr;
  }

  [[nodiscard]] auto starts_with(chain_erasure const& other) const -> bool
  {
    assert(same_type(other));
    assert(has_starts_with());
    return this->table().starts_with(this->storage(), other.storage());
  }

  [[nodiscard]] auto has_mismatch() const noexcept -> bool
  {
    return this->table().mismatch != nullptr;
  }

  [[nodiscard]] auto mismatch(chain_erasure const& other) const -> std::size_t
  {
    assert(same_type(other));
    assert(has_mismatch());
    return this->table().mismatch(this->storage(), other.storage());
  }

private:
  template <typename T>
  static inline auto const vtable_for = [] {
    auto value = chain_erasure_vtable{
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
      .get = [](void const* p, std::size_t i) -> block_header {
        return std::ranges::begin(object<T>(p))[i];
      },
    };

    if constexpr (requires(T const& v) { v.starts_with(v); }) {
      using actual_result =
        std::remove_cvref_t<decltype(std::declval<T const&>().starts_with(
          std::declval<T const&>()))>;
      static_assert(
        std::same_as<actual_result, bool>,
        "bitcoin::chain fast-path customization T::starts_with(T const&) "
        "must return bool");

      value.starts_with = [](void const* storage, void const* other) -> bool {
        return object<T>(storage).starts_with(object<T>(other));
      };
    }

    if constexpr (requires(T const& v) { v.mismatch(v); }) {
      using actual_mismatch_result =
        std::remove_cvref_t<decltype(std::declval<T const&>().mismatch(
          std::declval<T const&>()))>;
      using expected_mismatch_result =
        std::ranges::mismatch_result<std::ranges::iterator_t<T const>,
                                     std::ranges::iterator_t<T const>>;
      static_assert(
        std::same_as<actual_mismatch_result, expected_mismatch_result>,
        "bitcoin::chain fast-path customization T::mismatch(T const&) must "
        "return std::ranges::mismatch_result<iterator_t<const T>, "
        "iterator_t<const T>>");

      value.mismatch = [](void const* storage,
                          void const* other) -> std::size_t {
        auto const& lhs = object<T>(storage);
        auto const& rhs = object<T>(other);
        auto const result = lhs.mismatch(rhs);

        auto const lhs_index = result.in1 - std::ranges::begin(lhs);
        auto const rhs_index = result.in2 - std::ranges::begin(rhs);
        assert(lhs_index == rhs_index);
        assert(lhs_index >= decltype(lhs_index){0});
        assert(rhs_index >= decltype(rhs_index){0});

        auto const index = static_cast<std::size_t>(lhs_index);
        assert(index <= std::ranges::size(lhs));
        assert(index <= std::ranges::size(rhs));
        return index;
      };
    }

    return value;
  }();
};

} // namespace bitcoin::detail
