// SPDX-License-Identifier: BSL-1.0

module;

#include <concepts>
#include <ranges>

export module bitcoin:concepts;

import :customization_points;
import :vocabulary;

export namespace bitcoin {

template <typename T>
concept chain_view = std::ranges::view<T>
  && std::ranges::sized_range<T>
  && std::ranges::random_access_range<T>
  && std::convertible_to<std::ranges::range_reference_t<T>, block_header>;

template <typename T>
concept coin = requires(T const& c) {
  { bitcoin::value(c) } -> std::convertible_to<amount>;
  { bitcoin::output_script(c) } -> std::convertible_to<script_ref>;
  { bitcoin::funding_height(c) } -> std::convertible_to<std::size_t>;
  { bitcoin::is_coinbase(c) } -> std::convertible_to<bool>;
};

template <typename T>
concept coin_index =
  coin<typename T::mapped_type> && requires(T const& m, outpoint const& p) {
    {
      m.lookup(p)
    } -> std::convertible_to<std::optional<typename T::mapped_type>>;
  };

} // namespace bitcoin
