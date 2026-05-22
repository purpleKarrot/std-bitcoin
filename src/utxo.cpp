// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/utxo.hpp>

#include <cassert>
#include <utility>

#include <bitcoin/amount.hpp>
#include <bitcoin/coin.hpp>
#include <bitcoin/detail/spent_coins_erasure.hpp>
#include <bitcoin/script.hpp>

namespace bitcoin {

tx_spent_coins::tx_spent_coins(detail::spent_coins_erasure impl,
                               std::size_t tx_index)
  : _impl(std::move(impl))
  , _tx_index(tx_index)
{
}

auto tx_spent_coins::operator[](std::size_t input_index) const -> coin
{
  assert(input_index < size());
  return _impl.get_coin(_tx_index, input_index);
}

auto tx_spent_coins::size() const -> std::size_t
{
  assert(_tx_index < _impl.size());
  return _impl.inputs_size(_tx_index);
}

auto any_spent_coins::operator[](std::size_t tx_index) const -> tx_spent_coins
{
  assert(tx_index < size());
  return {_impl, tx_index};
}

auto any_spent_coins::size() const -> std::size_t
{
  return _impl.size();
}

} // namespace bitcoin
