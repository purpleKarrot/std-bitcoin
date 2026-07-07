// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <type_traits>
#include <utility>

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

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(block_header const& h, Chain&& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(h, std::forward<Chain>(chain), now);
  }

  //
  // Block
  //

  [[nodiscard]] auto operator()(block const& b) const { return verify(b); }

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(block const& b, Chain&& chain,
                                std::chrono::sys_seconds now) const
  {
    return verify(b, std::forward<Chain>(chain), now);
  }

  template <typename Chain, typename Coins>
    requires chain_view<std::remove_cvref_t<Chain>>
    && coin_index<std::remove_cvref_t<Coins>>
  [[nodiscard]] auto operator()(block const& b, Chain&& chain,
                                std::chrono::sys_seconds now,
                                Coins&& coins) const
  {
    return verify(b, std::forward<Chain>(chain), now,
                  std::forward<Coins>(coins));
  }

  //
  // Transaction
  //

  [[nodiscard]] auto operator()(transaction const& tx) const
  {
    return verify(tx);
  }

  template <typename Chain>
    requires chain_view<std::remove_cvref_t<Chain>>
  [[nodiscard]] auto operator()(transaction const& tx, Chain&& chain) const
  {
    return verify(tx, std::forward<Chain>(chain));
  }

  template <typename Chain, typename Coins>
    requires chain_view<std::remove_cvref_t<Chain>>
    && coin_index<std::remove_cvref_t<Coins>>
  [[nodiscard]] auto operator()(transaction const& tx, Chain&& chain,
                                Coins&& coins) const
  {
    return verify(tx, std::forward<Chain>(chain), std::forward<Coins>(coins));
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
                                Prevouts&& prevouts) const
  {
    return verify(script, value, tx, input_index, flags,
                  std::forward<Prevouts>(prevouts));
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
