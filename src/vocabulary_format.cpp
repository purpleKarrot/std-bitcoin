// SPDX-License-Identifier: BSL-1.0

module;

#include <format>
#include <string_view>

module bitcoin;
import legacy;

auto std::formatter<bitcoin::outpoint>::format(bitcoin::outpoint const& obj,
                                               std::format_context& ctx) const
  -> std::format_context::iterator
{
  auto const str = legacy::convert_outpoint(obj).ToString();
  return std::formatter<string_view>::format(str, ctx);
}

auto std::formatter<bitcoin::tx_input>::format(bitcoin::tx_input const& obj,
                                               std::format_context& ctx) const
  -> std::format_context::iterator
{
  auto const str = legacy::convert_txin(obj).ToString();
  return std::formatter<string_view>::format(str, ctx);
}

auto std::formatter<bitcoin::tx_output>::format(bitcoin::tx_output const& obj,
                                                std::format_context& ctx) const
  -> std::format_context::iterator
{
  auto const str = legacy::convert_txout(obj).ToString();
  return std::formatter<string_view>::format(str, ctx);
}

auto std::formatter<bitcoin::transaction>::format(
  bitcoin::transaction const& obj, std::format_context& ctx) const
  -> std::format_context::iterator
{
  auto const str = legacy::convert_tx(obj).ToString();
  return std::formatter<string_view>::format(str, ctx);
}

auto std::formatter<bitcoin::block>::format(bitcoin::block const& obj,
                                            std::format_context& ctx) const
  -> std::format_context::iterator
{
  auto const str = legacy::convert_block(obj).ToString();
  return std::formatter<string_view>::format(str, ctx);
}
