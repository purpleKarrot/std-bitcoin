// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <ranges>

#include <bitcoin/block_header.hpp>
#include <bitcoin/chain_view.hpp>
#include <bitcoin/detail/iterator.hpp>

namespace bitcoin {

class any_chain_view : public std::ranges::view_interface<any_chain_view>
{
public:
  using iterator = detail::iterator<any_chain_view>;
  using const_iterator = iterator;

  any_chain_view() noexcept = default;
  any_chain_view(any_chain_view const& other);
  any_chain_view(any_chain_view&& other) noexcept;

  auto operator=(any_chain_view const& other) -> any_chain_view&;
  auto operator=(any_chain_view&& other) noexcept -> any_chain_view&;

  ~any_chain_view();

  template <typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, any_chain_view>
             && chain_view<std::remove_cvref_t<T>>)
  explicit any_chain_view(T&& object)
  {
    if (std::ranges::empty(object)) {
      return;
    }

    using value_type = std::remove_cvref_t<T>;
    construct<value_type>(std::forward<T>(object), vtable_for<value_type>);
  }

  [[nodiscard]] auto operator[](std::size_t index) const -> block_header;

  [[nodiscard]] auto begin() const noexcept -> iterator;
  [[nodiscard]] auto end() const noexcept(noexcept(size())) -> iterator;

  [[nodiscard]] auto size() const -> std::size_t;
  [[nodiscard]] auto height() const -> std::size_t;

  [[nodiscard]] auto mismatch(any_chain_view const& other) const
    -> std::ranges::mismatch_result<iterator, iterator>;

  [[nodiscard]] auto starts_with(any_chain_view const& prefix) const -> bool;

private:
  struct vtable
  {
    void (*copy)(void const* src, void* dst);
    void (*move)(void* src, void* dst) noexcept;
    void (*destroy)(void* storage) noexcept;
    std::size_t (*size)(void const* storage);
    block_header (*get)(void const* storage, std::size_t index);
    bool (*starts_with)(void const* storage, void const* other);
    std::size_t (*mismatch)(void const* storage, void const* other);
  };

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

  static constexpr std::size_t storage_size = 3 * sizeof(void*);
  static_assert(storage_size >= sizeof(std::shared_ptr<void const>));

  static constexpr std::size_t storage_align = alignof(std::max_align_t);
  static_assert(storage_align >= alignof(std::shared_ptr<void const>));

  template <typename T>
  static constexpr bool stores_inline = (sizeof(T) <= storage_size)
    && (alignof(T) <= storage_align)
    && std::copy_constructible<T>
    && std::is_nothrow_move_constructible_v<T>;

  template <typename T>
  using storage_t =
    std::conditional_t<stores_inline<T>, T, std::shared_ptr<T const>>;

  template <typename T>
  [[nodiscard]] static auto raw_storage(void* p) noexcept -> storage_t<T>*
  {
    return reinterpret_cast<storage_t<T>*>(p);
  }

  template <typename T>
  [[nodiscard]] static auto raw_storage(void const* p) noexcept
    -> storage_t<T> const*
  {
    return reinterpret_cast<storage_t<T> const*>(p);
  }

  template <typename T>
  [[nodiscard]] static auto storage(void* p) noexcept -> storage_t<T>*
  {
    return std::launder(raw_storage<T>(p));
  }

  template <typename T>
  [[nodiscard]] static auto storage(void const* p) noexcept
    -> storage_t<T> const*
  {
    return std::launder(raw_storage<T>(p));
  }

  template <typename T>
  [[nodiscard]] static auto object(void const* p) noexcept -> T const&
  {
    if constexpr (stores_inline<T>) {
      return *storage<T>(p);
    }
    else {
      return **storage<T>(p);
    }
  }

  template <typename T, typename U>
  static void construct_storage(void* p, U&& value)
  {
    if constexpr (stores_inline<T>) {
      std::construct_at(raw_storage<T>(p), std::forward<U>(value));
    }
    else {
      std::construct_at(raw_storage<T>(p),
                        std::make_shared<T>(std::forward<U>(value)));
    }
  }

  template <typename T, typename U>
  void construct(U&& value, vtable const& table)
  {
    construct_storage<T>(storage(), std::forward<U>(value));
    _vtable = std::addressof(table);
  }

  [[nodiscard]] auto storage() noexcept -> void*;
  [[nodiscard]] auto storage() const noexcept -> void const*;

  [[nodiscard]] auto table() const noexcept -> vtable const&;

  [[nodiscard]] auto same_type(any_chain_view const& other) const noexcept
    -> bool;

  template <typename T>
  static inline auto const vtable_for = [] {
    auto value = vtable{
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
        "bitcoin::chain_view fast-path customization T::starts_with(T const&) "
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
        "bitcoin::chain_view fast-path customization T::mismatch(T const&) "
        "must return std::ranges::mismatch_result<iterator_t<const T>, "
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

  alignas(storage_align) std::array<std::byte, storage_size> _storage{};
  vtable const* _vtable = std::addressof(empty_vtable);
};

} // namespace bitcoin
