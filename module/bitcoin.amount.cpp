// SPDX-License-Identifier: BSL-1.0

module;

#include <cstdint>

#include <mp-units/framework.h>

export module bitcoin:amount;

namespace bitcoin {

constexpr struct amount_dimension final : mp_units::base_dimension<"btc_amt">
{
} amount_dimension;

constexpr struct bitcoin_amount final
  : mp_units::quantity_spec<amount_dimension>
{
} bitcoin_amount;

export namespace units {

constexpr struct satoshi final
  : mp_units::named_unit<"sat", mp_units::kind_of<bitcoin_amount>>
{
} satoshi;

constexpr struct btc final
  : mp_units::named_unit<"BTC", mp_units::mag<100'000'000> * satoshi>
{
} btc;

} // namespace units

export using amount = mp_units::quantity<units::satoshi, std::int64_t>;

} // namespace bitcoin
