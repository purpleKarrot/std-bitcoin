// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdint>
#include <format>

#include <bitcoin/block.hpp>
#include <bitcoin/block_header.hpp>
#include <bitcoin/transaction.hpp>

namespace bitcoin {

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
// verification_status verify(block_header const& header, chain);

verification_status verify(bitcoin::block const& block);
// verification_status verify(bitcoin::block const& block, chain);
// verification_status verify(bitcoin::block const& block, chain, coins);

verification_status verify(bitcoin::transaction const& tx);
// verification_status verify(bitcoin::transaction const& tx, chain);
// verification_status verify(bitcoin::transaction const& tx, chain, coins);

} // namespace bitcoin

template <>
struct std::formatter<bitcoin::verification_status>
  : std::formatter<std::string_view>
{
  std::format_context::iterator format(bitcoin::verification_status status,
                                       std::format_context& ctx) const;
};
