// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace bitcoin::detail {

template <typename Policy>
class erasure
{
public:
  using vtable = Policy::vtable;

  erasure() noexcept = default;

  erasure(erasure const& other)
    : _vtable(other._vtable)
  {
    _vtable->copy(other.storage(), storage());
  }

  erasure(erasure&& other) noexcept
    : _vtable(
        std::exchange(other._vtable, std::addressof(Policy::empty_vtable)))
  {
    _vtable->move(other.storage(), storage());
  }

  auto operator=(erasure const& other) -> erasure&
  {
    if (this != std::addressof(other)) {
      _vtable->destroy(_storage);
      _vtable = other._vtable;
      _vtable->copy(other.storage(), storage());
    }
    return *this;
  }

  auto operator=(erasure&& other) noexcept -> erasure&
  {
    if (this != std::addressof(other)) {
      _vtable->destroy(_storage);
      _vtable =
        std::exchange(other._vtable, std::addressof(Policy::empty_vtable));
      _vtable->move(other.storage(), storage());
    }
    return *this;
  }

  ~erasure() { _vtable->destroy(storage()); }

  [[nodiscard]] auto same_type(erasure const& other) const noexcept -> bool
  {
    return _vtable == other._vtable;
  }

protected:
  static constexpr std::size_t storage_size = 3 * sizeof(void*);
  static_assert(storage_size >= sizeof(std::shared_ptr<void const>));

  static constexpr std::size_t storage_align = alignof(std::max_align_t);
  static_assert(storage_align >= alignof(std::shared_ptr<void const>));

  template <typename T>
  static constexpr bool stores_inline =
    sizeof(T) <= storage_size && alignof(T) <= storage_align &&
    std::copy_constructible<T> && std::is_nothrow_move_constructible_v<T>;

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

  [[nodiscard]] auto storage() noexcept -> void* { return _storage.data(); }

  [[nodiscard]] auto storage() const noexcept -> void const*
  {
    return _storage.data();
  }

  [[nodiscard]] auto table() const noexcept -> vtable const&
  {
    return *_vtable;
  }

private:
  alignas(storage_align) std::array<std::byte, storage_size> _storage{};
  vtable const* _vtable = std::addressof(Policy::empty_vtable);
};

} // namespace bitcoin::detail
