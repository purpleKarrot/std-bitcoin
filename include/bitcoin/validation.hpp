// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstdint>
#include <format>

#include <beman/any_view/any_view.hpp>
#include <beman/any_view/any_view_options.hpp>

#include <bitcoin/amount.hpp>
#include <bitcoin/block.hpp>
#include <bitcoin/block_header.hpp>
#include <bitcoin/chain_view.hpp>
#include <bitcoin/coin_index.hpp>
#include <bitcoin/consensus_parameters.hpp>
#include <bitcoin/script.hpp>
#include <bitcoin/transaction.hpp>
#include <bitcoin/validation_flags.hpp>
#include <bitcoin/validation_status.hpp>

namespace bitcoin {

class block;
class transaction;
struct block_header;

class verifier
{
public:
  constexpr explicit verifier(consensus_parameters const& params) noexcept
    : _params(&params)
  {
  }

  //
  // Block header
  //

  [[nodiscard]] auto operator()(block_header const& h) const
  {
    return verify(h);
  }

  template <chain_view Chain>
  [[nodiscard]] auto operator()(block_header const& h, Chain const& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(h, chain, now);
  }

  //
  // Block
  //

  [[nodiscard]] auto operator()(block const& b) const { return verify(b); }

  template <chain_view Chain>
  [[nodiscard]] auto operator()(block const& b, Chain const& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(b, chain, now);
  }

  template <chain_view Chain, coin_index Coins>
  [[nodiscard]] auto operator()(block const& b, Chain const& chain,
                                std::chrono::sys_seconds now,
                                Coins const& coins) const
  {
    return verify(b, chain, now, coins);
  }

  //
  // Transaction
  //

  [[nodiscard]] auto operator()(transaction const& tx) const
  {
    return verify(tx);
  }

  template <chain_view Chain>
  [[nodiscard]] auto operator()(transaction const& tx, Chain const& chain) const
  {
    return verify(tx, chain);
  }

  template <chain_view Chain, coin_index Coins>
  [[nodiscard]] auto operator()(transaction const& tx, Chain const& chain,
                                Coins const& coins) const
  {
    return verify(tx, chain, coins);
  }

  //
  // Script
  //

  [[nodiscard]] auto operator()(script_ref script, amount value,
                                transaction const& tx, std::size_t input_index,
                                validation_flags flags) const
  {
    return verify(script, value, tx, input_index, flags, {});
  }

  template <typename Prevouts>
  [[nodiscard]] auto operator()(script_ref script, amount value,
                                transaction const& tx, std::size_t input_index,
                                validation_flags flags,
                                Prevouts const& prevouts) const
  {
    return verify(script, value, tx, input_index, flags, prevouts);
  }

private:
  static constexpr auto sized_random_access =
    beman::any_view::any_view_options::sized
    | beman::any_view::any_view_options::random_access;

  template <typename T>
  using any_sized_random_access_view =
    beman::any_view::any_view<T const, sized_random_access>;

  using any_chain_view = any_sized_random_access_view<block_header>;
  using any_prevouts_view = any_sized_random_access_view<tx_output>;

  //
  // Block header
  //

  validation_status verify(block_header const& header) const;
  validation_status verify(block_header const& header, any_chain_view chain,
                           std::chrono::sys_seconds now) const;

  //
  // Block
  //

  validation_status verify(bitcoin::block const& block) const;
  validation_status verify(bitcoin::block const& block, any_chain_view chain,
                           std::chrono::sys_seconds now) const;
  // validation_status verify(bitcoin::block const& block, chain, now, coins);

  //
  // Transaction
  //

  validation_status verify(bitcoin::transaction const& tx) const;
  validation_status verify(bitcoin::transaction const& tx,
                           any_chain_view chain) const;
  // validation_status verify(bitcoin::transaction const& tx, chain,
  // coins)const;

  //
  // Script
  //

  validation_status verify(script_ref script, amount value,
                           transaction const& tx, std::size_t input_index,
                           validation_flags flags,
                           any_prevouts_view prevouts) const;

  //
  // Parameters
  //

  consensus_parameters const* _params;
};

inline constexpr auto verify = verifier{mainnet::params};

} // namespace bitcoin
