// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <mp-units/ext/format.h>
#include <mp-units/framework.h>

namespace bitcoin {
namespace detail {

inline constexpr struct amount_dimension final
  : mp_units::base_dimension<"btc_amt">
{
} amount_dimension;

inline constexpr struct bitcoin_amount final
  : mp_units::quantity_spec<amount_dimension>
{
} bitcoin_amount;

} // namespace detail

namespace units {

inline constexpr struct satoshi final
  : mp_units::named_unit<"sat", mp_units::kind_of<detail::bitcoin_amount>>
{
} satoshi;

inline constexpr struct btc final
  : mp_units::named_unit<"BTC", mp_units::mag<100'000'000> * satoshi>
{
} btc;

} // namespace units

using amount = mp_units::quantity<units::satoshi, std::int64_t>;

} // namespace bitcoin
