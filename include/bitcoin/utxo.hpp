// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include <bitcoin/coin.hpp>
#include <bitcoin/detail/iterator.hpp>
#include <bitcoin/detail/spent_coins_erasure.hpp>

namespace bitcoin {
class block;

template <typename T>
concept coin_view =
  std::ranges::view<T> && std::ranges::random_access_range<T> &&
  std::ranges::sized_range<T> && requires(T const& object, std::size_t index) {
    { object[index] } -> std::convertible_to<coin>;
  };

template <typename T>
concept spent_coins =
  std::ranges::view<T> && std::ranges::random_access_range<T> &&
  std::ranges::sized_range<T> && requires(T const& object, std::size_t index) {
    requires coin_view<std::remove_cvref_t<decltype(object[index])>>;
  };

class tx_spent_coins : public detail::random_access_view<tx_spent_coins>
{
public:
  tx_spent_coins() noexcept = default;

  [[nodiscard]] auto operator[](std::size_t input_index) const -> coin;
  [[nodiscard]] auto size() const -> std::size_t;

private:
  friend class any_spent_coins;

  tx_spent_coins(detail::spent_coins_erasure impl, std::size_t tx_index);

  detail::spent_coins_erasure _impl;
  std::size_t _tx_index = 0;
};

class any_spent_coins : public detail::random_access_view<any_spent_coins>
{
public:
  any_spent_coins() noexcept = default;

  template <spent_coins T>
    requires(!std::same_as<std::remove_cvref_t<T>, any_spent_coins>)
  any_spent_coins(T&& object)
    : _impl(std::forward<T>(object))
  {
  }

  [[nodiscard]] auto operator[](std::size_t tx_index) const -> tx_spent_coins;
  [[nodiscard]] auto size() const -> std::size_t;

private:
  detail::spent_coins_erasure _impl;
};

class utxo_access
{
public:
  // Returns true if every spending input in this block references an existing
  // unspent output.
  [[nodiscard]] virtual bool previous_outputs_unspent(
    bitcoin::block const& block) const = 0;

  // Returns true if none of this block's transaction outputs already exist as
  // unspent outputs (BIP30).
  [[nodiscard]] virtual bool outpoints_unique(
    bitcoin::block const& block) const = 0;

  // Returns the spent coins for this block.
  [[nodiscard]] virtual any_spent_coins spent_coins(
    bitcoin::block const& block) const = 0;

protected:
  ~utxo_access() = default;
};

} // namespace bitcoin
