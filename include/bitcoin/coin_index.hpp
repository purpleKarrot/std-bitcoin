// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <concepts>

#include <bitcoin/coin.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

template <typename T>
concept coin_index = requires(T const& m, outpoint p) {
  { m.lookup(p) } -> std::convertible_to<std::optional<coin>>;
};

} // namespace bitcoin
