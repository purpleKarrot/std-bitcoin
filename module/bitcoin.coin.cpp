// SPDX-License-Identifier: BSL-1.0

module;

#include <cstddef>
#include <utility>

export module bitcoin:coin;

import :amount;
import :script;

namespace bitcoin {

template <class Coin>
  requires requires(Coin const& c) {
    { c.amount() } -> std::convertible_to<bitcoin::amount>;
  }
auto coin_value(Coin const& c) -> bitcoin::amount
{
  return c.amount();
}

template <class Coin>
  requires requires(Coin const& c) {
    { c.output_script() } -> std::convertible_to<bitcoin::script_ref>;
  }
auto coin_output_script(Coin const& c) -> bitcoin::script_ref
{
  return c.output_script();
}

template <class Coin>
  requires requires(Coin const& c) {
    { c.funding_height() } -> std::convertible_to<std::size_t>;
  }
auto coin_funding_height(Coin const& c) -> std::size_t
{
  return c.funding_height();
}

template <class Coin>
  requires requires(Coin const& c) {
    { c.is_coinbase() } -> std::convertible_to<bool>;
  }
auto coin_is_coinbase(Coin const& c) -> bool
{
  return c.is_coinbase();
}

export template <typename T>
concept coin = requires(T const& c) {
  { coin_value(c) } -> std::convertible_to<bitcoin::amount>;
  { coin_output_script(c) } -> std::convertible_to<bitcoin::script_ref>;
  { coin_funding_height(c) } -> std::convertible_to<std::size_t>;
  { coin_is_coinbase(c) } -> std::convertible_to<bool>;
};

struct coin_impl
{
  bitcoin::amount value;
  bitcoin::script_ref output_script;
  std::size_t funding_height;
  bool is_coinbase;
};

auto coin_value(coin_impl const& c) -> bitcoin::amount
{
  return c.value;
}

auto coin_output_script(coin_impl const& c) -> bitcoin::script_ref
{
  return c.output_script;
}

auto coin_funding_height(coin_impl const& c) -> std::size_t
{
  return c.funding_height;
}

auto coin_is_coinbase(coin_impl const& c) -> bool
{
  return c.is_coinbase;
}

static_assert(coin<coin_impl>);

constexpr auto make_coin_impl = [](coin auto const& c) {
  return coin_impl{
    .value = coin_value(c),
    .output_script = coin_output_script(c),
    .funding_height = coin_funding_height(c),
    .is_coinbase = coin_is_coinbase(c),
  };
};

} // namespace bitcoin
