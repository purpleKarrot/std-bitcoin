// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>

#include <bitcoin/amount.hpp>
#include <bitcoin/hash_id.hpp>

namespace bitcoin {

struct consensus_parameters
{
  block_hash genesis_block_hash = block_hash{std::array{
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x19}, std::byte{0xd6}, std::byte{0x68},
    std::byte{0x9c}, std::byte{0x08}, std::byte{0x5a}, std::byte{0xe1},
    std::byte{0x65}, std::byte{0x83}, std::byte{0x1e}, std::byte{0x93},
    std::byte{0x4f}, std::byte{0xf7}, std::byte{0x63}, std::byte{0xae},
    std::byte{0x46}, std::byte{0xa2}, std::byte{0xa6}, std::byte{0xc1},
    std::byte{0x72}, std::byte{0xb3}, std::byte{0xf1}, std::byte{0xb6},
    std::byte{0x0a}, std::byte{0x8c}, std::byte{0xe2}, std::byte{0x6f},
  }};

  // Proof of work
  std::uint32_t proof_of_work_limit = 0x1d00ffff;
  std::size_t retarget_interval = 2016;
  std::int64_t target_timespan_seconds = 14 * 24 * 60 * 60;

  // Block subsidy
  amount initial_subsidy = amount{50, units::btc};
  std::size_t halving_interval = 210'000;

  // Coinbase maturity (confirmations required before a coinbase output may be
  // spent)
  std::size_t coinbase_maturity = 100;

  // Block limits
  std::size_t max_block_weight = 4'000'000;
  std::size_t max_block_serialized_size = 4'000'000;
  std::size_t max_block_sigops = 80'000;

  // Transaction limits
  std::size_t max_tx_weight = 400'000;
  std::size_t max_tx_sigops = 20'000;

  // Locktime and relative locktime
  bool locktime_enabled = true;
  bool relative_locktime_enabled = true;

  // BIP-34 height commitment
  bool bip34_enabled = true;
  std::size_t bip34_activation_height = 227'931;

  // SegWit
  bool segwit_enabled = true;
  std::size_t segwit_activation_height = 481'824;
};

namespace mainnet {
inline constexpr auto params = consensus_parameters{};
}

namespace testnet {

inline constexpr auto params = consensus_parameters{
  .proof_of_work_limit = 0x1d00ffff,
  .retarget_interval = 2016,
  .target_timespan_seconds = 14 * 24 * 60 * 60,
  .initial_subsidy = amount{50, units::btc},
  .halving_interval = 210'000,
  .coinbase_maturity = 100,
  .max_block_weight = 4'000'000,
  .max_block_serialized_size = 4'000'000,
  .max_block_sigops = 80'000,
  .max_tx_weight = 400'000,
  .max_tx_sigops = 20'000,
  .locktime_enabled = true,
  .relative_locktime_enabled = true,
  .bip34_enabled = true,
  .bip34_activation_height = 0,
  .segwit_enabled = true,
  .segwit_activation_height = 834'624,
};

} // namespace testnet

namespace signet {

inline constexpr auto params = consensus_parameters{
  .proof_of_work_limit = 0x1e0377ae,
  .retarget_interval = 2016,
  .target_timespan_seconds = 14 * 24 * 60 * 60,
  .initial_subsidy = amount{50, units::btc},
  .halving_interval = 210'000,
  .coinbase_maturity = 100,
  .max_block_weight = 4'000'000,
  .max_block_serialized_size = 4'000'000,
  .max_block_sigops = 80'000,
  .max_tx_weight = 400'000,
  .max_tx_sigops = 20'000,
  .locktime_enabled = true,
  .relative_locktime_enabled = true,
  .bip34_enabled = true,
  .bip34_activation_height = 0,
  .segwit_enabled = true,
  .segwit_activation_height = 0,
};

} // namespace signet

namespace regtest {

inline constexpr auto params = consensus_parameters{
  .proof_of_work_limit = 0x207fffff,
  .retarget_interval = std::numeric_limits<std::size_t>::max(),
  .target_timespan_seconds = 14 * 24 * 60 * 60,
  .initial_subsidy = amount{50, units::btc},
  .halving_interval = 150,
  .coinbase_maturity = 100,
  .max_block_weight = 4'000'000,
  .max_block_serialized_size = 4'000'000,
  .max_block_sigops = 80'000,
  .max_tx_weight = 400'000,
  .max_tx_sigops = 20'000,
  .locktime_enabled = true,
  .relative_locktime_enabled = true,
  .bip34_enabled = true,
  .bip34_activation_height = 0,
  .segwit_enabled = true,
  .segwit_activation_height = 0,
};

} // namespace regtest
} // namespace bitcoin
