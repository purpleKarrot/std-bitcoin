// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bitcoin/hash_id.hpp>

namespace bitcoin {

class block;
class outpoint;
class tx_input;
class tx_output;
class transaction;

// [bitcoin.pred.txid]
[[nodiscard]] bool is_coinbase(txid const& t) noexcept;

// [bitcoin.pred.outpoint]
[[nodiscard]] bool is_coinbase(outpoint const& o) noexcept;

// [bitcoin.pred.tx_input]
[[nodiscard]] bool signals_rbf(tx_input const& i) noexcept;

[[nodiscard]] bool has_relative_locktime(tx_input const& i) noexcept;

// [bitcoin.pred.tx_output]
[[nodiscard]] bool is_unspendable(tx_output const& o) noexcept;

// [bitcoin.pred.transaction]
[[nodiscard]] bool is_coinbase(transaction const& t);

[[nodiscard]] bool is_segwit(transaction const& t);

[[nodiscard]] bool locktime_is_height(transaction const& t) noexcept;

[[nodiscard]] bool locktime_is_time(transaction const& t) noexcept;

// [bitcoin.pred.block]
[[nodiscard]] bool has_coinbase(block const& b);

} // namespace bitcoin
