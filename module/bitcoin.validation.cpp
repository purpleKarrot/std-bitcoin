// SPDX-License-Identifier: BSL-1.0

module;

#include <chrono>
#include <cstdint>
#include <format>
#include <type_traits>

export module bitcoin:validation;

import :concepts;
import :consensus_parameters;
import :type_erasure;
import :vocabulary;

export namespace bitcoin {

class validation_status
{
public:
  constexpr validation_status() = default;
  constexpr validation_status(std::uint8_t status)
    : _status(status)
  {
  }

  [[nodiscard]] constexpr auto ok() const { return _status == 0; }
  constexpr explicit operator bool() const { return ok(); }

private:
  std::uint8_t _status = 0;
};

enum class validation_flags : std::uint_least32_t
{
  none = 0,
  p2sh = 1U << 0,                 // BIP16
  dersig = 1U << 2,               // BIP66
  nulldummy = 1U << 4,            // BIP147
  checklocktimeverify = 1U << 9,  // BIP65
  checksequenceverify = 1U << 10, // BIP112
  witness = 1U << 11,             // BIP141
  taproot = 1U << 17,             // BIPs 341 & 342
  all = p2sh
    | dersig
    | nulldummy
    | checklocktimeverify
    | checksequenceverify
    | witness
    | taproot,
};

[[nodiscard]] constexpr auto operator|(validation_flags l,
                                       validation_flags r) noexcept
{
  return validation_flags(static_cast<int>(l) | static_cast<int>(r));
}

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
  //
  // Block header
  //

  [[nodiscard]] validation_status verify(block_header const& header) const;
  [[nodiscard]] validation_status verify(block_header const& header,
                                         type_erasure::any_chain_view chain,
                                         std::chrono::sys_seconds now) const;

  //
  // Block
  //

  [[nodiscard]] validation_status verify(bitcoin::block const& block) const;
  [[nodiscard]] validation_status verify(bitcoin::block const& block,
                                         type_erasure::any_chain_view chain,
                                         std::chrono::sys_seconds now) const;
  [[nodiscard]] validation_status verify(
    bitcoin::block const& block, type_erasure::any_chain_view chain,
    std::chrono::sys_seconds now, type_erasure::coin_index_ref coins) const;

  //
  // Transaction
  //

  [[nodiscard]] validation_status verify(bitcoin::transaction const& tx) const;
  [[nodiscard]] validation_status verify(
    bitcoin::transaction const& tx, type_erasure::any_chain_view chain) const;
  // validation_status verify(bitcoin::transaction const& tx, chain,
  // coins)const;

  //
  // Script
  //

  [[nodiscard]] validation_status verify(
    script_ref script, amount value, transaction const& tx,
    std::size_t input_index, validation_flags flags,
    type_erasure::any_prevouts_view prevouts) const;

  //
  // Parameters
  //

  consensus_parameters const* _params;
};

constexpr auto verify = verifier{mainnet::params};

namespace testnet {
constexpr auto verify = verifier{params};
} // namespace testnet

namespace signet {
constexpr auto verify = verifier{params};
} // namespace signet

namespace regtest {
constexpr auto verify = verifier{params};
} // namespace regtest

} // namespace bitcoin

export template <>
struct std::formatter<bitcoin::validation_flags>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    format_parse_context::iterator const it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(bitcoin::validation_flags flags, format_context& ctx) const
    -> format_context::iterator;
};

export template <>
struct std::formatter<bitcoin::validation_status>
{
  constexpr auto parse(format_parse_context& ctx)
  {
    format_parse_context::iterator const it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw format_error("unexpected format specifier");
    }
    return it;
  }

  auto format(bitcoin::validation_status status, format_context& ctx) const
    -> format_context::iterator;
};
