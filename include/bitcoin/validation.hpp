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
#include <bitcoin/chain.hpp>
#include <bitcoin/script.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

enum class verification_flags : std::uint_least32_t
{
  none = 0,
  p2sh = 1U << 0,                 // BIP16
  dersig = 1U << 2,               // BIP66
  nulldummy = 1U << 4,            // BIP147
  checklocktimeverify = 1U << 9,  // BIP65
  checksequenceverify = 1U << 10, // BIP112
  witness = 1U << 11,             // BIP141
  taproot = 1U << 17,             // BIPs 341 & 342
  all = p2sh | dersig | nulldummy | checklocktimeverify | checksequenceverify |
    witness | taproot,
};

[[nodiscard]] constexpr verification_flags operator|(
  verification_flags l, verification_flags r) noexcept
{
  return verification_flags(static_cast<int>(l) | static_cast<int>(r));
}

class verification_status
{
public:
  constexpr verification_status() = default;
  constexpr verification_status(std::uint8_t status)
    : _status(status)
  {
  }

  constexpr explicit operator bool() const { return _status == 0; }

private:
  std::uint8_t _status = 0;
};

verification_status verify(block_header const& header);
verification_status verify(block_header const& header, any_chain_view chain,
                           std::chrono::sys_seconds now);

verification_status verify(bitcoin::block const& block);
verification_status verify(bitcoin::block const& block, any_chain_view chain,
                           std::chrono::sys_seconds now);
// verification_status verify(bitcoin::block const& block, chain, now, coins);

verification_status verify(bitcoin::transaction const& tx);
verification_status verify(bitcoin::transaction const& tx,
                           any_chain_view chain);
// verification_status verify(bitcoin::transaction const& tx, chain, coins);

verification_status verify(
  script_ref script, amount value, transaction const& tx,
  std::size_t input_index, verification_flags flags,
  beman::any_view::any_view<tx_output const,
                            beman::any_view::any_view_options::random_access |
                              beman::any_view::any_view_options::sized>
    spent_outputs = {});

} // namespace bitcoin

template <>
struct std::formatter<bitcoin::verification_flags>
  : std::formatter<std::string_view>
{
  std::format_context::iterator format(bitcoin::verification_flags flags,
                                       std::format_context& ctx) const;
};

template <>
struct std::formatter<bitcoin::verification_status>
  : std::formatter<std::string_view>
{
  std::format_context::iterator format(bitcoin::verification_status status,
                                       std::format_context& ctx) const;
};
